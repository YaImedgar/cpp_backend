#pragma once
#include <chrono>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "gascooker.h"
#include "ingredients.h"

/*
Класс Хот-дог.
*/
using namespace std::literals;

class HotDog {
  private:
    void CheckInvariant(Clock::duration duration, Clock::duration min,
                        Clock::duration max, std::string_view name) {
        if (duration < min || duration > max) {
            std::ostringstream oss;
            oss << "Invalid " << name << " cook duration" << std::endl;
            throw std::invalid_argument(oss.str());
        }
    }

  public:
    // Минимальная и максимальная длительность выпекания хлеба и приготовления
    // сосиски для использования в хот-доге. Если время приготовления
    // ингредиентов хот-дога не попадают в эти интервалы, хот-дог считается
    // забракованным

    constexpr static Clock::duration MIN_SAUSAGE_COOK_DURATION =
        Milliseconds{1500};
    constexpr static Clock::duration MAX_SAUSAGE_COOK_DURATION =
        Milliseconds{2000};
    constexpr static Clock::duration MIN_BREAD_COOK_DURATION =
        Milliseconds{1000};
    constexpr static Clock::duration MAX_BREAD_COOK_DURATION =
        Milliseconds{1500};

    HotDog(int id, std::shared_ptr<Sausage> sausage,
           std::shared_ptr<Bread> bread)
        : id_{id}, sausage_{std::move(sausage)}, bread_{std::move(bread)} {
        CheckInvariant(sausage_->GetCookDuration(), MIN_SAUSAGE_COOK_DURATION,
                       MAX_SAUSAGE_COOK_DURATION, "sausage"sv);
        CheckInvariant(bread_->GetBakingDuration(), MIN_BREAD_COOK_DURATION,
                       MAX_BREAD_COOK_DURATION, "bread"sv);
    }

    int GetId() const noexcept { return id_; }

    const Sausage &GetSausage() const noexcept { return *sausage_; }

    const Bread &GetBread() const noexcept { return *bread_; }

  private:
    int id_;
    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bread_;
};
