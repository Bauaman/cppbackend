//#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>


#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logger.h"
#include "model.h"


using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace fs = std::filesystem;


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
    logger::Logger logger;
    logger.Init();

    try {
        
        model::Game game = json_loader::LoadGame(argv[1]);
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        std::string root_dir = argv[2];
        fs::path root = fs::weakly_canonical(fs::path(root_dir));
        model::GameServer server(ioc, root, game);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, &logger](const sys::error_code ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                logger::LogExit(0);
            }
        });

        auto api_strand = net::make_strand(ioc);

        auto handler = std::make_shared<http_handler::RequestHandler>(ioc, api_strand, server);
        //http_handler::LoggingRequestHandler<http_handler::RequestHandler> logging_handler{*handler};
        http_handler::LoggingRequestHandler logging_handler{[handler](auto&& endpoint, auto&& req, auto&& send){
            (*handler)(std::forward<decltype(endpoint)>(endpoint),
                       std::forward<decltype(req)>(req),
                       std::forward<decltype(send)>(send));
            }};

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        boost::json::object add_data;
        add_data["port"] = port;
        add_data["address"] = address.to_string();
        //BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server started";
        logger::LogMessageInfo(add_data, "server started"s);

        /*http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });*/
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        
        logger::LogExit(EXIT_FAILURE, &ex);
        return EXIT_FAILURE;
    }

    logger::LogExit(0);
}