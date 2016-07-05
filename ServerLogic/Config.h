#pragma once

namespace SERVER_CONFIG {
	const short port = 19999; // server binding port
	const int max_packet_size = 1024; // maximum packet size

	const int max_connection = 128; // maximum client connection size
	const int ip_length = 32; // IP Address length

	


	// 공부좀 더 해야할 설정
	const int backlog_size = 32; // listen backlog count ------------- 더 공부해보기

	const short MaxClientSockOptRecvBufferSize = 10240;
	const short MaxClientSockOptSendBufferSize = 10240;
	const short MaxClientRecvBufferSize = 8192;
	const short MaxClientSendBufferSize = 8192;
}