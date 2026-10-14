// SPDX-License-Identifier: MIT
#pragma once

#include "AdsLib.h"
#include <fructose/fructose.h>
#include <system_error>
#include <cstdio>
#include <thread>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#include <vector>
#else
#include <cerrno>
#include <fcntl.h>
#endif

#ifdef __linux__
#include <dirent.h>
#include <csignal>
#include <pthread.h>
#include <fstream>
#include <future>
#include <sstream>
#endif

struct TestSocketHandle {
	SOCKET fd;

	TestSocketHandle()
		: fd(socket(AF_INET, SOCK_STREAM, 0))
	{
		if (fd == INVALID_SOCKET) {
			throw std::system_error(WSAGetLastError(),
						std::system_category());
		}
	}
	~TestSocketHandle()
	{
		closesocket(fd);
	}
	TestSocketHandle(const TestSocketHandle &) = delete;
	TestSocketHandle &operator=(const TestSocketHandle &) = delete;
};

struct TestSocketLibrary {
	TestSocketLibrary()
	{
		if (InitSocketLibrary()) {
			throw std::runtime_error("WSAStartup failed");
		}
	}
	~TestSocketLibrary()
	{
		WSACleanup();
	}
};

struct TestTcpListener {
	TestSocketLibrary library;
	TestSocketHandle socket;
	sockaddr_in address{};
	std::string endpoint;

	TestTcpListener(int backlog = 8)
	{
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		socklen_t length = sizeof(address);
		if (bind(socket.fd, reinterpret_cast<sockaddr *>(&address),
			 length) ||
		    getsockname(socket.fd,
				reinterpret_cast<sockaddr *>(&address),
				&length) ||
		    listen(socket.fd, backlog)) {
			throw std::system_error(WSAGetLastError(),
						std::system_category());
		}
		endpoint =
			"127.0.0.1:" + std::to_string(ntohs(address.sin_port));
	}

	void ClosePeer()
	{
		const auto peer = accept(socket.fd, nullptr, nullptr);
		if (peer == INVALID_SOCKET) {
			throw std::system_error(WSAGetLastError(),
						std::system_category());
		}
		closesocket(peer);
	}
};

struct TestConnectedSocket : TcpSocket {
	using TcpSocket::TcpSocket;

	SOCKET Handle() const
	{
		return m_Socket;
	}
};

#if defined(_WIN32) && !defined(__CYGWIN__)
struct TestSaturatedListener : TestTcpListener {
	std::vector<std::unique_ptr<TestSocketHandle> > fillers;

	TestSaturatedListener()
		: TestTcpListener(1)
	{
		// Probe the actual backlog instead of assuming Windows honors its
		// requested size. Keep all peers alive, including the pending one.
		for (int i = 0; i < 32; ++i) {
			fillers.emplace_back(new TestSocketHandle);
			const auto fd = fillers.back()->fd;
			u_long mode = 1;
			if (ioctlsocket(fd, FIONBIO, &mode)) {
				throw std::system_error(WSAGetLastError(),
							std::system_category());
			}
			if (!connect(fd, reinterpret_cast<sockaddr *>(&address),
				     sizeof(address))) {
				continue;
			}
			const auto error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				throw std::system_error(error,
							std::system_category());
			}
			fd_set writable, errors;
			FD_ZERO(&writable);
			FD_ZERO(&errors);
			FD_SET(fd, &writable);
			FD_SET(fd, &errors);
			timeval timeout{ 0, 100000 };
			const auto ready = select(0, nullptr, &writable,
						  &errors, &timeout);
			if (!ready) {
				return;
			}
			int socketError = 0;
			socklen_t length = sizeof(socketError);
			if (ready == SOCKET_ERROR ||
			    getsockopt(fd, SOL_SOCKET, SO_ERROR,
				       reinterpret_cast<char *>(&socketError),
				       &length)) {
				throw std::system_error(WSAGetLastError(),
							std::system_category());
			}
			if (socketError) {
				throw std::system_error(socketError,
							std::system_category());
			}
		}
		throw std::runtime_error(
			"could not saturate loopback accept queue");
	}
};
#endif

#ifdef __linux__
struct TestSignalInterruptions {
	struct sigaction previous;
	std::thread sender;
	static void Ignore(int)
	{
	}
	TestSignalInterruptions()
	{
		struct sigaction action{};
		action.sa_handler = Ignore;
		sigemptyset(&action.sa_mask);
		if (sigaction(SIGUSR1, &action, &previous)) {
			throw std::runtime_error("sigaction failed");
		}
		const auto target = pthread_self();
		sender = std::thread([target]() {
			for (int i = 0; i < 5; ++i) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(20));
				pthread_kill(target, SIGUSR1);
			}
		});
	}
	~TestSignalInterruptions()
	{
		sender.join();
		sigaction(SIGUSR1, &previous, nullptr);
	}
};
#endif

struct TestSockets : fructose::test_base<TestSockets> {
	using Clock = std::chrono::steady_clock;

	void testReachable(const std::string &)
	{
		TestTcpListener server;
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		TestConnectedSocket client(addresses.get(),
					   Clock::now() +
						   std::chrono::seconds(2));
		fructose_assert_eq(0x7f000001U, client.Connect());
#if !(defined(_WIN32) && !defined(__CYGWIN__))
		fructose_assert(
			!(fcntl(client.Handle(), F_GETFL, 0) & O_NONBLOCK));
#endif
		const auto peer = accept(server.socket.fd, nullptr, nullptr);
		fructose_assert(peer != INVALID_SOCKET);
		const char payload = 'x';
		fructose_assert_eq(1, send(peer, &payload, 1, 0));
		uint8_t received = 0;
		timeval timeout{ 1, 0 };
		fructose_assert_eq(1U, client.read(&received, 1, &timeout));
		fructose_assert_eq('x', received);
		closesocket(peer);
	}

	void testRefused(const std::string &)
	{
		// Keep the port bound without listening, so it cannot be reused.
		TestSocketLibrary library;
		TestSocketHandle bound;
		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		socklen_t length = sizeof(address);
		fructose_assert_eq(0,
				   bind(bound.fd,
					reinterpret_cast<sockaddr *>(&address),
					length));
		fructose_assert_eq(
			0, getsockname(bound.fd,
				       reinterpret_cast<sockaddr *>(&address),
				       &length));
		auto addresses = bhf::ads::GetListOfAddresses(
			"127.0.0.1:" + std::to_string(ntohs(address.sin_port)));
		const auto start = Clock::now();
#if defined(_WIN32) && !defined(__CYGWIN__)
		// Winsock retries refused loopback connections for about two seconds.
		const auto budget = std::chrono::seconds(5);
#else
		const auto budget = std::chrono::seconds(2);
#endif
		try {
			TcpSocket client(addresses.get(), start + budget);
			fructose_assert(false);
		} catch (const std::system_error &error) {
#if defined(_WIN32) && !defined(__CYGWIN__)
			fructose_assert_eq(WSAECONNREFUSED,
					   error.code().value());
#else
			fructose_assert_eq(ECONNREFUSED, error.code().value());
#endif
		}
		fructose_assert(Clock::now() - start < budget);
	}

	void testExpiredDeadline(const std::string &)
	{
		TestTcpListener server;
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		try {
			TcpSocket client(addresses.get(),
					 Clock::now() -
						 std::chrono::milliseconds(1));
			fructose_assert(false);
		} catch (const std::system_error &error) {
#if defined(_WIN32) && !defined(__CYGWIN__)
			fructose_assert_eq(WSAETIMEDOUT, error.code().value());
#else
			fructose_assert_eq(ETIMEDOUT, error.code().value());
#endif
		}
	}

	void testAddRoute(const std::string &)
	{
		TestTcpListener server;
		const AmsNetId netId{ 127, 0, 0, 1, 1, 1 };
		// Both entry points and the opt-out retain connection reuse/refcounts.
		fructose_assert_eq(
			0, AdsAddRouteEx(netId, server.endpoint.c_str(), 1000));
		server.ClosePeer();
		fructose_assert_eq(0,
				   AdsAddRoute(netId, server.endpoint.c_str()));
		fructose_assert_eq(
			0, AdsAddRouteEx(netId, server.endpoint.c_str(), 0));
		AdsDelRoute(netId);
		AdsDelRoute(netId);
		AdsDelRoute(netId);
		fructose_assert_eq(
			0, AdsAddRouteEx(netId, server.endpoint.c_str(), 0));
		server.ClosePeer();
		AdsDelRoute(netId);
	}

#if defined(_WIN32) && !defined(__CYGWIN__)
	static DWORD CountHandles()
	{
		DWORD count = 0;
		if (!GetProcessHandleCount(GetCurrentProcess(), &count)) {
			throw std::runtime_error(
				"cannot inspect socket cleanup");
		}
		return count;
	}

	void testWindowsConnectTimeout(const std::string &)
	{
		TestSaturatedListener server;
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		DWORD handles = 0;
		for (int i = 0; i < 5; ++i) {
			const auto start = Clock::now();
			try {
				TcpSocket client(
					addresses.get(),
					start + std::chrono::milliseconds(50));
				fructose_assert(false);
			} catch (const std::system_error &error) {
				fructose_assert_eq(WSAETIMEDOUT,
						   error.code().value());
			}
			const auto elapsed = Clock::now() - start;
			fructose_assert(elapsed >=
					std::chrono::milliseconds(50));
			fructose_assert(elapsed < std::chrono::seconds(1));
			if (i == 0) {
				// Exclude lazy Winsock/CRT initialization from the
				// baseline, then check four more failed attempts.
				handles = CountHandles();
			}
		}
		fructose_assert_eq(handles, CountHandles());
	}

	void testWindowsSharedDeadline(const std::string &)
	{
		TestSaturatedListener server;
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		struct addrinfo candidates[4];
		for (int i = 0; i < 4; ++i) {
			candidates[i] = *addresses;
			candidates[i].ai_next = i == 3 ? nullptr :
							 &candidates[i + 1];
		}
		const auto start = Clock::now();
		try {
			TcpSocket client(
				candidates,
				start + std::chrono::milliseconds(150));
			fructose_assert(false);
		} catch (const std::system_error &error) {
			fructose_assert_eq(WSAETIMEDOUT, error.code().value());
		}
		const auto elapsed = Clock::now() - start;
		fructose_assert(elapsed >= std::chrono::milliseconds(150));
		fructose_assert(elapsed < std::chrono::milliseconds(400));
	}

	void testWindowsRouteRecovery(const std::string &)
	{
		TestSaturatedListener server;
		const AmsNetId netId{ 127, 0, 0, 1, 1, 5 };
		const auto start = Clock::now();
		fructose_assert_eq(GLOBALERR_TARGET_PORT,
				   AdsAddRouteEx(netId, server.endpoint.c_str(),
						 100));
		const auto elapsed = Clock::now() - start;
		fructose_assert(elapsed >= std::chrono::milliseconds(100));
		fructose_assert(elapsed < std::chrono::seconds(1));
		// The timed-out attempt must not leave this NetId pending or mapped.
		TestTcpListener reachable;
		fructose_assert_eq(0, AdsAddRouteEx(netId,
						    reachable.endpoint.c_str(),
						    1000));
		reachable.ClosePeer();
		AdsDelRoute(netId);
	}
#endif

#ifdef __linux__
	static size_t CountDescriptors()
	{
		auto directory = std::unique_ptr<DIR, int (*)(DIR *)>(
			opendir("/proc/self/fd"), closedir);
		if (!directory) {
			throw std::runtime_error(
				"cannot inspect socket cleanup");
		}
		size_t count = 0;
		while (readdir(directory.get())) {
			++count;
		}
		return count;
	}

	void testConnectTimeout(const std::string &)
	{
		// On Linux a full accept queue drops subsequent SYNs. This creates a
		// loopback black hole without firewall/routing changes or privileges.
		TestTcpListener server(0);
		TestSocketHandle filler;
		fructose_assert_eq(0, connect(filler.fd,
					      reinterpret_cast<sockaddr *>(
						      &server.address),
					      sizeof(server.address)));
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		const auto descriptors = CountDescriptors();
		for (int i = 0; i < 4; ++i) {
			const auto start = Clock::now();
			try {
				TcpSocket client(
					addresses.get(),
					start + std::chrono::milliseconds(50));
				fructose_assert(false);
			} catch (const std::system_error &error) {
				fructose_assert_eq(ETIMEDOUT,
						   error.code().value());
			}
			const auto elapsed = Clock::now() - start;
			fructose_assert(elapsed >=
					std::chrono::milliseconds(50));
			fructose_assert(elapsed < std::chrono::seconds(1));
		}
		fructose_assert_eq(descriptors, CountDescriptors());
	}

	void testSharedDeadlineAndInterruptions(const std::string &)
	{
		TestTcpListener server(0);
		TestSocketHandle filler;
		fructose_assert_eq(0, connect(filler.fd,
					      reinterpret_cast<sockaddr *>(
						      &server.address),
					      sizeof(server.address)));
		auto addresses = bhf::ads::GetListOfAddresses(server.endpoint);
		// Several resolved addresses must share a single budget, even if
		// signals repeatedly interrupt the wait for the first address.
		struct addrinfo candidates[4];
		for (int i = 0; i < 4; ++i) {
			candidates[i] = *addresses;
			candidates[i].ai_next = i == 3 ? nullptr :
							 &candidates[i + 1];
		}
		TestSignalInterruptions interruptions;
		const auto start = Clock::now();
		try {
			TcpSocket client(
				candidates,
				start + std::chrono::milliseconds(150));
			fructose_assert(false);
		} catch (const std::system_error &error) {
			fructose_assert_eq(ETIMEDOUT, error.code().value());
		}
		const auto elapsed = Clock::now() - start;
		fructose_assert(elapsed >= std::chrono::milliseconds(150));
		fructose_assert(elapsed < std::chrono::milliseconds(400));
	}

	void testAddRouteTimeoutAndRecovery(const std::string &)
	{
		TestTcpListener server(0);
		TestSocketHandle filler;
		fructose_assert_eq(0, connect(filler.fd,
					      reinterpret_cast<sockaddr *>(
						      &server.address),
					      sizeof(server.address)));
		const AmsNetId netId{ 127, 0, 0, 1, 1, 2 };
		const auto descriptors = CountDescriptors();
		const auto start = Clock::now();
		fructose_assert_eq(GLOBALERR_TARGET_PORT,
				   AdsAddRouteEx(netId, server.endpoint.c_str(),
						 100));
		fructose_assert(Clock::now() - start >=
				std::chrono::milliseconds(100));
		fructose_assert(Clock::now() - start < std::chrono::seconds(1));
		fructose_assert_eq(descriptors, CountDescriptors());
		// A failed construction must remove the router's pending attempt.
		TestTcpListener reachable;
		fructose_assert_eq(0, AdsAddRouteEx(netId,
						    reachable.endpoint.c_str(),
						    1000));
		AdsDelRoute(netId);
		fructose_assert_eq(descriptors + 1, CountDescriptors());
	}

	void testPendingRouteDeadline(const std::string &)
	{
		TestTcpListener server(0);
		TestSocketHandle filler;
		fructose_assert_eq(0, connect(filler.fd,
					      reinterpret_cast<sockaddr *>(
						      &server.address),
					      sizeof(server.address)));
		const AmsNetId netId{ 127, 0, 0, 1, 1, 3 };
		auto first = std::async(std::launch::async, [&]() {
			return AdsAddRouteEx(netId, server.endpoint.c_str(),
					     500);
		});
		// Observe SYN_SENT instead of guessing when the first attempt started.
		char destination[32];
		std::snprintf(destination, sizeof(destination), "%08X:%04X",
			      static_cast<unsigned int>(
				      server.address.sin_addr.s_addr),
			      static_cast<unsigned int>(
				      ntohs(server.address.sin_port)));
		bool pending = false;
		const auto waitUntil = Clock::now() + std::chrono::seconds(1);
		do {
			std::ifstream connections("/proc/net/tcp");
			for (std::string line;
			     std::getline(connections, line);) {
				std::istringstream row(line);
				std::string index, local, remote, state;
				row >> index >> local >> remote >> state;
				pending |= remote == destination &&
					   state == "02";
			}
			if (!pending) {
				std::this_thread::sleep_for(
					std::chrono::milliseconds(2));
			}
		} while (!pending && Clock::now() < waitUntil);
		fructose_assert(pending);
		const auto start = Clock::now();
		fructose_assert_eq(GLOBALERR_TARGET_PORT,
				   AdsAddRouteEx(netId, server.endpoint.c_str(),
						 50));
		fructose_assert(Clock::now() - start >=
				std::chrono::milliseconds(50));
		fructose_assert(Clock::now() - start <
				std::chrono::milliseconds(300));
		// Other NetIds must remain usable while this connection is pending.
		TestTcpListener reachable;
		const AmsNetId other{ 127, 0, 0, 1, 1, 4 };
		fructose_assert_eq(0, AdsAddRouteEx(other,
						    reachable.endpoint.c_str(),
						    1000));
		AdsDelRoute(other);
		fructose_assert_eq(GLOBALERR_TARGET_PORT, first.get());
		fructose_assert_eq(0, AdsAddRouteEx(netId,
						    reachable.endpoint.c_str(),
						    1000));
		AdsDelRoute(netId);
	}
#endif
};

inline int RunSocketTests()
{
	TestSockets tests;
	tests.add_test("testReachable", &TestSockets::testReachable);
	tests.add_test("testRefused", &TestSockets::testRefused);
	tests.add_test("testExpiredDeadline",
		       &TestSockets::testExpiredDeadline);
	tests.add_test("testAddRoute", &TestSockets::testAddRoute);
#if defined(_WIN32) && !defined(__CYGWIN__)
	tests.add_test("testWindowsConnectTimeout",
		       &TestSockets::testWindowsConnectTimeout);
	tests.add_test("testWindowsSharedDeadline",
		       &TestSockets::testWindowsSharedDeadline);
	tests.add_test("testWindowsRouteRecovery",
		       &TestSockets::testWindowsRouteRecovery);
#endif
#ifdef __linux__
	tests.add_test("testConnectTimeout", &TestSockets::testConnectTimeout);
	tests.add_test("testAddRouteTimeoutAndRecovery",
		       &TestSockets::testAddRouteTimeoutAndRecovery);
	tests.add_test("testSharedDeadlineAndInterruptions",
		       &TestSockets::testSharedDeadlineAndInterruptions);
	tests.add_test("testPendingRouteDeadline",
		       &TestSockets::testPendingRouteDeadline);
#endif
	return tests.run();
}
