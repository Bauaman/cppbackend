#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>


#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;


namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <base directory name>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        Logger logger;
        logger.InitLogging("game_server.log");

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                http_server::ReportServerExit(0);
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler handler{game};

        http_handler::LoggingRequestHandler<http_handler::RequestHandler> loggingHandler{handler};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        std::string root_dir = argv[2];
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        boost::json::object add_data;
        add_data["port"] = port;
        add_data["address"] = address.to_string();
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server started";

        http_server::ServeHttp(ioc, {address, port}, [&loggingHandler, root_dir](auto&& req, auto&& send, const std::string& address_string) {
        loggingHandler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), root_dir, address_string);
        }, root_dir);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        //std::cerr << ex.what() << std::endl;
        http_server::ReportServerExit(EXIT_FAILURE, &ex);
        return EXIT_FAILURE;
    }
}
