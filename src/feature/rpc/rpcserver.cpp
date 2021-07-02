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

typedef enum : uint8_t { READ_MEM, WRITE_MEM } rpc_opt_t;

void handle_conn(FILE* conn) {
	rpc_opt_t opt;
	fread(&opt, sizeof(rpc_opt_t), 1, conn);
	switch (opt) {
		// inp: [ READ_MEM ] | [a] [d] [d] [r] | [s] [i] [z] [e]
		// out: [ mem[addr] ] ... [ mem[addr + size - 1] ]
	case READ_MEM: {
		uint32_t addr = 0;
		fread(&addr, sizeof(uint32_t), 1, conn);
		uint32_t size = 0;
		fread(&size, sizeof(uint32_t), 1, conn);
		fwrite(&size, sizeof(uint32_t), 1, conn);
		for (int i = 0; i < size; i++) {
			uint8_t value = m_core->busRead8(m_core, addr + i);
			fwrite(&value, sizeof(uint8_t), 1, conn);
		}
		break;
	}
	default: {
		uint32_t size = 8;
		fwrite(&size, sizeof(uint32_t), 1, conn);
		fwrite("RECOG_NO", 1, 8, conn);
		break;
	}
	}
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

		FILE* conn = fdopen(accept(fd, (struct sockaddr*) &address, &addr_size), "r+b");
		setvbuf(conn, NULL, _IOFBF, 1024);

		fwrite(BANNER, 1, sizeof(BANNER) - 1, conn);
		fflush(conn);
		// handle_conn(sock);
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