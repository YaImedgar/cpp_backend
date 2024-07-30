#include "audio.h"
#include <iostream>
#include <unordered_map>
#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <bitset>

#include <boost/asio.hpp>

using namespace std::literals;

namespace {

enum class type : bool {
    kClient,
    kServer,
};

const std::unordered_map<std::string_view, type> programm_type{
    {"client"sv, type::kClient},
    {"server"sv, type::kServer},
};

}

constexpr auto LOCAL_HOST = "127.0.0.1";

namespace net = boost::asio;
using net::ip::udp;

void StartServer(uint16_t port){
    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        Player player(ma_format_u8, 1);

        const auto frame_size = player.GetFrameSize();

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, 2 << 15> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::string_view sv(recv_buf.data(), recv_buf.size());

            std::cout << "Recived data from client\nDATA=["sv << std::bitset<16>(sv) << "]\n";

            Recorder::RecordingResult rec_result{
                .data  = {recv_buf.begin(), recv_buf.end()},
                .frames = recv_buf.size() / frame_size,
            };

            player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
            std::cout << "Playing done" << std::endl;

            // Отправляем ответ на полученный endpoint, игнорируя ошибку.
            // На этот раз не отправляем перевод строки: размер датаграммы будет получен автоматически.
            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("OK"sv), remote_endpoint, 0, ignored_error);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartClient(uint16_t port){
    Recorder recorder(ma_format_u8, 1);
    std::string str;

    std::cout << "Press Enter to record message..." << std::endl;
    std::getline(std::cin, str);

    auto rec_result = recorder.Record(65000, 1.5s);
    std::cout << "Recording done" << std::endl;
    
    const auto size_in_bytes = recorder.GetFrameSize() * rec_result.frames;
    try {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет. 
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        const auto data = rec_result.data.data();

        boost::system::error_code ec;
        auto endpoint = udp::endpoint(net::ip::make_address(LOCAL_HOST, ec), port);
        socket.send_to(net::buffer(data, size_in_bytes), endpoint);

        // std::cout << "Sending data to server\nDATA=["sv << std::format("{:b}"sv, std::join(rec_result.data, "")) << "]\n";

        // Получаем данные и endpoint.
        std::array<char, 2 << 15> recv_buf;
        udp::endpoint sender_endpoint;
        size_t size = socket.receive_from(net::buffer(recv_buf), sender_endpoint);

        std::cout << "Server responded "sv << std::string_view(recv_buf.data(), size) << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 3 || !programm_type.contains(argv[1])){
        std::cerr << "usage ./radio {client/server} {port}\n";
        return 1;
    }

    const uint16_t port = std::stoi(argv[2]);

    switch (programm_type.at(argv[1])){
        case type::kClient:
            StartClient(port);
            break;
        case type::kServer:
            StartServer(port);
            break;
    }

    return 0;
}
