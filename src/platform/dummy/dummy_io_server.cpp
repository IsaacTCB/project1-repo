#include "../io_server.h"

#include "dummy_common.h"

void IOServer::open_file() {
	m_state.shared_variable++;
	//PRINT_MSG(std::string(m_state.shared_variable));
}