#include <mgba/core/core.h>
#include <mgba/feature/rpcserver.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define PORT 2323
#define BANNER "RPC_MGBA"

namespace RPCServer {

mCore* m_core = nullptr;
std::thread* t_server = nullptr;
int fd;

typedef enum : uint8_t { READ_MEM, WRITE_MEM, EXIT } rpc_opt_t;

bool handle_conn(int conn) {
	rpc_opt_t opt = EXIT;
	uint32_t addr;
	uint32_t size;

	// if read fails, we drop to exit
	read(conn, &opt, sizeof(rpc_opt_t));
	switch (opt) {
		// inp: [ READ_MEM ] | [a] [d] [d] [r] | [s] [i] [z] [e]
		// out: [ mem[addr] ] ... [ mem[addr + size - 1] ]
	case READ_MEM: {
		read(conn, &addr, sizeof(uint32_t));
		read(conn, &size, sizeof(uint32_t));
		send(conn, &size, sizeof(uint32_t), 0);

		// buffer mem to prevent false finishes
		uint8_t* mem = new uint8_t[size];
		for (int i = 0; i < size; i++) {
			mem[i] = m_core->busRead8(m_core, addr + i);
		}
		send(conn, mem, size, 0);
		delete[] mem;

		break;
	}
	case WRITE_MEM: {
		read(conn, &addr, sizeof(uint32_t));
		read(conn, &size, sizeof(uint32_t));

		uint8_t* mem = new uint8_t[size];
		read(conn, mem, size);
		for (int i = 0; i < size; i++) {
			m_core->busWrite8(m_core, addr + i, mem[i]);
		}
		delete[] mem;

		size = 0;
		send(conn, &size, sizeof(uint32_t), 0);

		break;
	}

	case EXIT:
		return false;
	default:
		size = 8;
		send(conn, &size, sizeof(uint32_t), 0);
		send(conn, "RECOG_NO", size, 0);
		break;
	}
	return true;
}

void socket_start() {
	int opt;
	struct sockaddr_in address;
	socklen_t addr_size = sizeof(address);

	if (!(fd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	bind(fd, (struct sockaddr*) &address, sizeof(address));

	while (true) {
		listen(fd, 1);
		int conn = accept(fd, (struct sockaddr*) &address, &addr_size);
		send(conn, BANNER, sizeof(BANNER) - 1, 0);
		while (handle_conn(conn))
			;
		close(conn);
	}
}

void maybe_start_RPC(struct mCore* core) {
	if (t_server != nullptr) {
		return;
	}
	m_core = core;

	t_server = new std::thread([]() {
		socket_start();
		t_server = nullptr;
	});
}

void stop_RPC(bool wait) {
	if (wait && t_server != nullptr) {
		if (t_server->joinable()) {
			t_server->join();
		}
	}
}

}

void setCoreRPC(struct mCore* core) {
	RPCServer::maybe_start_RPC(core);
}