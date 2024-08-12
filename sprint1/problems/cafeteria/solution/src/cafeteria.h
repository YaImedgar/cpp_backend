#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <chrono>
#include <memory>
#include <optional>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using Timer = net::steady_timer;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

using namespace std::chrono;
class Logger {
  public:
    explicit Logger(int32_t id) : id_(std::to_string(id)) {}

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv
           << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

  private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class ThreadChecker {
  public:
    explicit ThreadChecker(std::atomic_int &counter) : counter_{counter} {}

    ThreadChecker(const ThreadChecker &) = delete;
    ThreadChecker &operator=(const ThreadChecker &) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

  private:
    std::atomic_int &counter_;
    int expected_counter_ = ++counter_;
};

class Order : public std::enable_shared_from_this<Order> {
  public:
    struct Properties {
        net::io_context &io;
        int id;
        HotDogHandler handler;
        std::shared_ptr<Bread> bread;
        std::shared_ptr<Sausage> sausage;
        std::shared_ptr<GasCooker> gas_cooker;
    };

  public:
    Order() = delete;
    Order(Order &&) = delete;
    Order(const Order &) = delete;
    Order &operator=(const Order &) = delete;
    Order &operator=(Order &&) = delete;
    ~Order() = default;
    explicit Order(Properties p)
        : io_(p.io), id_(p.id), handler_(p.handler), bread_(p.bread),
          sausage_(p.sausage), gas_cooker_(p.gas_cooker) {}

  public:
    void Execute() {
        logger_.LogMessage("Order has been started."sv);
        BakeBread();
        FrySausage();
    }

  private:
    void BakeBread() {
        logger_.LogMessage("Start MakeBread()"sv);
        bread_->StartBake(*gas_cooker_, [self = shared_from_this()] {
            self->bread_timer_.expires_after(1000ms);

            self->bread_timer_.async_wait(
                [self](boost::system::error_code) { self->OnBaked(); });
        });
    }

    void OnBaked() {
        bread_->StopBaking();
        logger_.LogMessage("Finish StopBaking()"sv);
        assert(bread_->IsCooked());
        CheckReadiness();
    }

    void FrySausage() {
        logger_.LogMessage("Start MakeSausage()"sv);
        sausage_->StartFry(*gas_cooker_, [self = shared_from_this()] {
            self->sausage_timer_.expires_after(1500ms);
            self->sausage_timer_.async_wait(
                [self](boost::system::error_code) { self->OnFryed(); });
        });
    }

    void OnFryed() {
        sausage_->StopFry();
        logger_.LogMessage("Finish StopFry()"sv);
        assert(sausage_->IsCooked());
        CheckReadiness();
    }

    void CheckReadiness() {
        if (!sausage_->IsCooked() || !bread_->IsCooked()) {
            return;
        }

        HandleResult();
    }

    void HandleResult() {
        logger_.LogMessage("Start HandleResult()"sv);
        handler_(Result<HotDog>(HotDog(id_, sausage_, bread_)));
        logger_.LogMessage("Finish HandleResult()"sv);
    }

  private:
    net::io_context &io_;
    int32_t id_;
    HotDogHandler handler_;
    std::shared_ptr<Bread> bread_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<GasCooker> gas_cooker_;
    Logger logger_{id_};
    Timer bread_timer_{io_, Milliseconds{1000}};
    Timer sausage_timer_{io_, Milliseconds{1500}};
    net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria : public std::enable_shared_from_this<Cafeteria> {
  public:
    explicit Cafeteria(net::io_context &io) : io_{io} {}

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет
    // готов. Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        std::make_shared<Order>(Order::Properties{
                                    .io = io_,
                                    .id = ++order_id_,
                                    .handler = std::move(handler),
                                    .bread = store_.GetBread(),
                                    .sausage = store_.GetSausage(),
                                    .gas_cooker = gas_cooker_,
                                })
            ->Execute();
    }

  private:
    net::io_context &io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    int32_t order_id_ = 0;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая
    // плита на 8 горелок Используйте её для приготовления ингредиентов
    // хот-дога. Плита создаётся с помощью make_shared, так как GasCooker
    // унаследован от enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
