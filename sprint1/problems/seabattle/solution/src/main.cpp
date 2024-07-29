#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void CheckErrorCode(const boost::system::error_code &ec,
                    const std::string_view message) {
    if (ec) {
        std::cout << message << std::endl;
        exit(1);
    }
}

void PrintFieldPair(const SeabattleField &left, const SeabattleField &right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket &socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket &socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()),
               ec);

    return !ec;
}

class SeabattleAgent {
    using Cell = std::pair<int, int>;

  public:
    SeabattleAgent(const SeabattleField &field) : my_field_(field) {}
    bool OtherTurn(tcp::socket &socket) {
        std::cout << "Waiting for the turn..." << std::endl;

        const std::string opponent_move = ReadMove(socket);

        std::cout << "Shot to " << opponent_move << std::endl;

        auto [x, y] = ParseMove(opponent_move).value();

        SeabattleField::ShotResult hit_or_kill = my_field_.Shoot(y, x);

        SendResult(socket, hit_or_kill);

        if (SeabattleField::ShotResult::MISS == hit_or_kill) {
            return true;
        }

        return false;
    }

    bool MyTurn(tcp::socket &socket) {
        std::cout << "Your turn: ";
        std::string my_move;
        std::cin >> my_move;
        auto parsed_move = ParseMove(my_move);

        if (!parsed_move) {
            std::cout << "Bad move, try again" << std::endl;
            return true;
        }

        auto [x, y] = *parsed_move;

        SendMove(socket, MoveToString({x, y}));
        SeabattleField::ShotResult shoot_result = ReadResult(socket);
        switch (shoot_result) {
            using enum SeabattleField::ShotResult;
        case MISS: {
            other_field_.MarkMiss(y, x);
            std::cout << "Miss!" << std::endl;
            return false;
        }
        case HIT: {
            other_field_.MarkHit(y, x);
            break;
        }
        case KILL: {
            other_field_.MarkKill(y, x);
            break;
        }
        }
        return true;
    }

    void StartGame(tcp::socket &socket, bool my_initiative) {
        while (!IsGameEnded()) {
            PrintFields();

            my_initiative = my_initiative ? MyTurn(socket) : OtherTurn(socket);
        }
    }

  private:
    static std::optional<Cell> ParseMove(const std::string_view &sv) {
        if (sv.size() != 2)
            return std::nullopt;

        int p1 = sv[0] - 'A';
        int p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 7)
            return std::nullopt;
        if (p2 < 0 || p2 > 7)
            return std::nullopt;

        return std::optional<Cell>{Cell{p1, p2}};
    }

    static std::string MoveToString(Cell move) {
        char buff[] = {
            static_cast<char>(move.first) + 'A',
            static_cast<char>(move.second) + '1',
        };
        return {buff, 2};
    }

    void PrintFields() const { PrintFieldPair(my_field_, other_field_); }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::string ReadMove(tcp::socket &socket) {
        return ReadExact<2>(socket).value();
    }

    SeabattleField::ShotResult ReadResult(tcp::socket &socket) {
        return static_cast<SeabattleField::ShotResult>(
            ReadExact<1>(socket).value().at(0));
    }

    void SendMove(tcp::socket &socket, std::string_view shoot) {
        WriteExact(socket, shoot);
    }

    void SendResult(tcp::socket &socket,
                    SeabattleField::ShotResult shoot_result) {
        char buff[] = {
            static_cast<char>(shoot_result),
        };

        WriteExact(socket, {buff, 1});
    }

  private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField &field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);
    CheckErrorCode(ec, "Can't accept connection"sv);

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField &field, const std::string &ip_str,
                 unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    CheckErrorCode(ec, "Wrong IP format"sv);

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);

    CheckErrorCode(ec, "Can't connect to server"sv);

    agent.StartGame(socket, true);
};

int main(int argc, const char **argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
