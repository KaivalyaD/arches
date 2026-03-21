#pragma once 
#include "stdafx.hpp"

#include "unit-memory-base.hpp"

#include "util/arbitration.hpp"

namespace Arches {
namespace Units {

class UnitTexture : public UnitMemoryBase
{
public:
	struct Configuration
	{
		uint            num_clients{1};
		uint8_t*        cheat_mem{nullptr};
		UnitMemoryBase* cache{nullptr};
		uint            cache_port{0};
	};

private:
	UnitMemoryBase* cache;
	uint cache_port;

	Cascade<MemoryRequest, RoundRobinArbiter<uint128_t>> _request_network;
	ReturnCascade _return_network;

	uint8_t* _cheat_mem;

	struct Sample
	{
		paddr_t texel_addrs[8];
		Texture2D::Texel texels[8];
		uint32_t pending_texels;
		MemoryRequest req;
	};

	std::unordered_map<uint32_t, Sample> _pending_samples;
	std::queue<MemoryRequest> _texel_fill_queue;
	LatencyFIFO<Sample> _filter_pipline;

public:
	UnitTexture(const Configuration& config) : UnitMemoryBase(), cache(config.cache), cache_port(config.cache_port), _cheat_mem(config.cheat_mem),
		_filter_pipline(10), _request_network(config.num_clients, 1), _return_network(1, config.num_clients)
	{
	}

	void clock_rise() override;
	void clock_fall() override;

	bool request_port_write_valid(uint port_index) override
	{
		return _request_network.is_write_valid(port_index);
	}

	void write_request(const MemoryRequest& request) override
	{
		_request_network.write(request, request.port);
	}

	bool return_port_read_valid(uint port_index) override
	{
		return _return_network.is_read_valid(port_index);
	}

	const MemoryReturn& peek_return(uint port_index) override
	{
		return _return_network.peek(port_index);
	}

	const MemoryReturn read_return(uint port_index) override
	{
		return _return_network.read(port_index);
	}
};

}
}