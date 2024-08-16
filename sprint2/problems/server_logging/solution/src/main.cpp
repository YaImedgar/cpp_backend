#include "sdk.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <thread>

#include "http_server.hpp"
#include "json_loader.hpp"
#include "logs.hpp"
#include "model.hpp"
#include "request_handler.hpp"

namespace {

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn> void RunWorkers(unsigned n, const Fn &fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

void OnExit(int exit_status, std::string message) {
    json::object data;

    data["code"] = exit_status;
    data["exception"] = std::move(message);

    BOOST_LOG_TRIVIAL(info)
        << logging::add_value(additional_data, std::move(data))
        << "server exited"sv;
}

} // namespace

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        std::cerr
            << "Usage: game_server <game-config-json> <static-content-directory>"sv
            << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // Проверяем папку со статическими файлами сервера
        const std::string static_content = argv[2];

        if (!std::filesystem::is_directory(static_content)) {
            std::cerr
                << R"(The path to <static-content-directory> is not a valid directory: \')"
                << static_content << "\'\n";
            return EXIT_FAILURE;
        }

        // Инициализация логов
        logs::init();

        // Загружаем карту из файла и строим модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec,
                                  [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
            OnExit(EXIT_SUCCESS, {});
        });

        // Создаём контекст HTTP-запросов
        http_handler::RequestHandler::Context context{
            .game = game,
            .static_content_directory_path = std::move(static_content),
        };

        // Создаём обработчик HTTP-запросов и связываем его с контекстом
        http_handler::RequestHandler handler(context);

        http_handler::LoggingRequestHandler logging_handler{handler};

        // Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(
            ioc, {address, port},
            [&logging_handler](auto &&endpoint, auto &&req, auto &&sender) {
                logging_handler(std::forward<decltype(endpoint)>(endpoint),
                                std::forward<decltype(req)>(req),
                                std::forward<decltype(sender)>(sender));
            });

        {
            json::object data;

            data["port"] = port;
            data["address"] = address.to_string();

            BOOST_LOG_TRIVIAL(info)
                << logging::add_value(additional_data, std::move(data))
                << "server started"sv;
        }

        // Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;

        OnExit(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
}
