/*
 *   ___ _ __  _ __  _ __ _____  ___   _
 *  / __| '_ \| '_ \| '__/ _ \ \/ / | | |
 * | (__| |_) | |_) | | | (_) >  <| |_| |
 *  \___| .__/| .__/|_|  \___/_/\_\\__, |
 *      |_|   |_|                  |___/ https://github.com/Thorek777/cpproxy
 *
 * For modern C++.
 * Required dependencies:
 *   https://proxycheck.io
 *   https://think-async.com/Asio
 *   https://github.com/nlohmann/json
 */

#pragma once

#include "asio/asio.hpp"
#include "json/json.hpp"

class cpproxy
{
	std::unordered_map<std::string, asio::ip::tcp::iostream> map_;

	static auto make_asio_iostream(const std::string& ip)
	{
		asio::ip::tcp::iostream stream("proxycheck.io", "http");

		stream << "POST /v2/" << ip << "?vpn=1&asn=1 HTTP/1.0\r\n";
		stream << "Host: proxycheck.io\r\n";
		stream << "Accept: */*\r\n";
		stream << "Content-Type: text/plain\r\n";
		stream << "Content-Length: 2\r\n";
		stream << "Connection: close\r\n\r\n";
		stream << "{}";

		return stream;
	}

	typedef struct s_ip
	{
		bool status;
		bool is_proxy;
		std::string json_error;
	} t_ip;

	static auto parse_by_json(const std::string& object)
	{
		t_ip ip = {};

		try
		{
			// ReSharper disable once CppTooWideScopeInitStatement
			const auto json = nlohmann::json::parse(object);

			for (const auto& it : json.items())
			{
				if (!it.value().is_object())
					ip.status = it.value() == "ok" ? true : false;

				if (it.value().is_object())
					ip.is_proxy = it.value().at("proxy") == "yes" ? true : false;
			}
		}
		catch (const nlohmann::json::exception& error)
		{
			ip.json_error = error.what();
			return ip;
		}

		return ip;
	}

	static auto cpproxy_exception(const std::string& message, const std::string& function)
	{
		return std::exception((function + std::string(" - ") + message).c_str());
	}

public:
	void add(const std::string& ip, const bool force_check = false)
	{
		map_.emplace(ip, !force_check ? asio::ip::tcp::iostream() : make_asio_iostream(ip));
	}

	void scan()
	{
		std::vector<std::jthread> threads;

		for (auto& [fst, snd] : map_)
		{
			if (snd.get() != -1)
				continue;

			threads.emplace_back([this, &fst, &snd]
				{
					snd = make_asio_iostream(fst);
				}
			);
		}
	}

	auto read(const std::string& ip)
	{
		if (const auto element = map_.find(ip); element == map_.end())
			throw cpproxy_exception("key (" + ip + ") not found.\n", __FUNCTION__);
		else
		{
			std::string string(std::istreambuf_iterator(element->second), {});
			string.erase(0, string.find('{'));
			return parse_by_json(string);
		}
	}
};
