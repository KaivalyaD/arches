#include "stdafx.hpp"
#include "unit-base.hpp"
#include "simulator/interconnects.hpp"
#include "simulator/transactions.hpp"

namespace Arches {
namespace Units {

class UnitGlobalWorkDispatcher : public UnitBase
{
public:
    struct Configuration
    {
        uint num_clients{1};        // Max = 256
        uint num_workers_per_client{32};
        
        uint work_element_size{1};  // In Bytes
        uint work_dispatch_size{1}; // Number of work elements to dispatch per client
        uint work_buffer_size{0};   // In Bytes

        UnitMemoryBase *mem_higher{nullptr};
        uint16_t        mem_higher_port{0};
        uint16_t        num_ports{1};

        vaddr_t work_buffer_ptr{0};
    };

private:
    class WorkIssueCrossbar : public CrossBar<WorkDispatch>
    {
    private:
        uint issued_clients = 0;

    public:
        WorkIssueCrossbar(uint num_clients, uint num_workers_per_client)
            : CrossBar<WorkDispatch>(num_clients, num_clients, 1, num_workers_per_client)
        {
        }

        uint get_sink(const WorkDispatch &work_index) override
        {
            uint sink = issued_clients % num_sinks();
            issued_clients++;
            return sink;
        }
    } work_issue_network;
    
    uint num_clients;
    uint work_elements_per_dispatch;
    uint work_element_size;
    uint work_dispatch_size;
    uint work_buffer_size;
    uint prefetch_requests_per_dispatch;

    UnitMemoryBase *mem;
    uint16_t        mem_port;
    uint16_t        num_ports;

    vaddr_t work_buffer_ptr{~0ull};
    uint prefetch_offset = 0u;

    struct PrefetchRequest
    {
        uint num_pending{0};
        vaddr_t vaddr{~0ull};
    };
    std::unordered_map<uint, PrefetchRequest> pending_prefetch;
    std::queue<WorkDispatch> pending_dispatch;
    
public:
    UnitGlobalWorkDispatcher(const Configuration &config):
        work_elements_per_dispatch(config.work_dispatch_size),
        work_element_size(config.work_element_size),
        work_buffer_size(config.work_buffer_size),
        num_clients(config.num_clients),
        work_issue_network(config.num_clients, config.num_workers_per_client),
        work_buffer_ptr(config.work_buffer_ptr),
        mem_port(config.mem_higher_port),
        num_ports(config.num_ports),
        mem(config.mem_higher)
    {
        work_dispatch_size = work_elements_per_dispatch * work_element_size;
        prefetch_requests_per_dispatch = (work_dispatch_size + MemoryRequest::MAX_SIZE - 1) / MemoryRequest::MAX_SIZE;
    }

	void clock_rise(void) override
	{
        // Collect work cachelines
        for(uint port_index = mem_port; port_index < mem_port + num_ports; ++port_index)
        {
            if(pending_prefetch.empty())
                break;

            if(!mem->return_port_read_valid(port_index))
                continue;
            
            MemoryReturn ret = mem->read_return(port_index);
            uint dispatch_index = ret.dst.pop(32);

            _assert(pending_prefetch.contains(dispatch_index));

            PrefetchRequest &pr = pending_prefetch[dispatch_index];
            if(--pr.num_pending == 0)
            {                    
                WorkDispatch wd;
                wd.element_type = WorkDispatch::Type::UINT32;
                wd.element_count = work_elements_per_dispatch;
                wd.vaddr = pr.vaddr;
                pending_prefetch.erase(dispatch_index);
                pending_dispatch.push(wd);
            }
        }
	}

	void clock_fall(void) override
	{
        // There is more work to prefetch, push requests on available ports to
        // prefetch the next cachelines worth of work
        for(uint port_index = mem_port; port_index < mem_port + num_ports; ++port_index)
        {
            // Finished prefetching all work
            if(prefetch_offset >= work_buffer_size)
                break;

            if(!mem->request_port_write_valid(port_index))
                continue;
            
            uint dispatch_index = prefetch_offset / work_dispatch_size;
            uint pending_prefetch_size = work_buffer_size - prefetch_offset;
            uint unit_prefetch_size = MemoryRequest::MAX_SIZE;

            MemoryRequest req;
            req.type = MemoryRequest::Type::LOAD;
            req.size = std::min(unit_prefetch_size, pending_prefetch_size);
            req.vaddr = work_buffer_ptr + prefetch_offset;
            req.dst.push(dispatch_index, 32);
            req.port = port_index;
            mem->write_request(req);

            if(!pending_prefetch.contains(dispatch_index))
            {
                // Prefetches for a new dispatch
                uint num_prefetches = (pending_prefetch_size < work_dispatch_size)
                    ? (pending_prefetch_size + unit_prefetch_size - 1) / unit_prefetch_size
                    : prefetch_requests_per_dispatch;

                pending_prefetch[dispatch_index] = {
                    .num_pending = num_prefetches,
                    .vaddr = req.vaddr
                };
            }

            prefetch_offset += unit_prefetch_size;
        }

        // Dispatch work to the clients
        for(uint port = 0; port < work_issue_network.num_sources(); ++port)
        {
            if(pending_dispatch.empty())
                break;

            if(!work_issue_network.is_write_valid(port))
                continue;

            const WorkDispatch &wd = pending_dispatch.front();
            work_issue_network.write(wd, port);
            pending_dispatch.pop();
        }

        work_issue_network.clock();
	}
};

}
}
