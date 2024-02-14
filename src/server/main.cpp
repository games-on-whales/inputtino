#include <server/rest.hpp>
#include <thread>


int main() {
    ServerState local_state = {.devices = {}};
    auto state = std::make_shared<immer::atom<ServerState>>(local_state);

    auto rest_ip = get_env("INPUTTINO_REST_IP", "0.0.0.0");
    auto rest_port = std::stoi(get_env("INPUTTINO_REST_PORT", "8080"));
    auto svr = setup_rest_server(state);
    auto svr_thread = std::thread([svr = std::move(svr), rest_ip, rest_port]() {
        svr->set_read_timeout(5, 0); // 5 seconds
        svr->set_write_timeout(5, 0); // 5 seconds
        svr->set_idle_interval(0, 100000); // 100 milliseconds
        svr->set_payload_max_length(1024 * 1024 * 512); // 512MB

        // This will wait here
        svr->listen(rest_ip, rest_port);
    });

    std::cout << "Server listening on http://" << rest_ip << ":" << rest_port << "" << std::endl;

    svr_thread.join();
    return 0;
}