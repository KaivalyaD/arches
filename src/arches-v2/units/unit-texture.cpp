#include "unit-texture.hpp"

namespace Arches { namespace Units {

void UnitTexture::clock_rise()
{
	_request_network.clock();

	if(_filter_pipline.is_write_valid() && cache->return_port_read_valid(cache_port))
	{
		MemoryReturn ret = cache->read_return(cache_port);
		Sample& sample = _pending_samples[ret.dst.raw];
		_assert(sample.texel_addrs[0] == ret.paddr);
		std::memcpy(&sample.texels[0], ret.data, sizeof(Texture2D::Texel));
		sample.pending_texels--;
		
		if(sample.pending_texels == 0)
			_filter_pipline.write(sample);
	}

	if(_request_network.is_read_valid(0))
	{
		MemoryRequest req = _request_network.read(0);
		req.dst.push(req.port, 8);

		Sample& sample = _pending_samples[req.dst.raw];
		sample.req = req;

		//nearest
		sample.pending_texels = 1;

		rtm::vec2 uv;
		std::memcpy(&uv, req.data, sizeof(rtm::vec2));
		Texture2D& texture = *(Texture2D*)&_cheat_mem[req.vaddr];

		rtm::uvec2 iuv = texture.get_iuv(uv);
		Texture2D::Texel* texel = texture.get_texel_addr(iuv);
		sample.texel_addrs[0] = (paddr_t)texel;

		MemoryRequest texel_req;
		texel_req.type = MemoryRequest::Type::LOAD;
		texel_req.paddr = sample.texel_addrs[0];
		texel_req.size = sizeof(Texture2D::Texel);
		texel_req.port = cache_port;
		texel_req.dst = req.dst;
		_texel_fill_queue.push(texel_req);
	}

}

void UnitTexture::clock_fall()
{
	_filter_pipline.clock();

	if(_filter_pipline.is_read_valid() && _return_network.is_write_valid(0))
	{
		Sample sample = _filter_pipline.read();

		MemoryReturn ret2(sample.req);
		ret2.size = sizeof(rtm::vec4);
		ret2.dst.pop(8);

		rtm::vec4 color = Texture2D::decode_texel(sample.texels[0]);
		std::memcpy(ret2.data, &color, sizeof(rtm::vec4));

		_return_network.write(ret2, 0);
	}

	if(_texel_fill_queue.size() && cache->request_port_write_valid(cache_port))
	{
		cache->write_request(_texel_fill_queue.front());
		_texel_fill_queue.pop();
	}

	_return_network.clock();
}

}
}