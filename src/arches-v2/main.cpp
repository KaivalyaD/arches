#include "stdafx.hpp"

#include "shared-utils.hpp"
#include "trax.hpp"
#include "dual-streaming.hpp"
#include "strata-rt.hpp"

//global verbosity flag
int arches_verbosity = 1;

int main(int argc, char* argv[])
{
	Arches::set_full_exe_name(argv[0]);
	Arches::SimulationConfig sim_config(argc, argv);
	sim_config.print();

	if (sim_config.get_string("arch_name") == "TRaX")
	{
		Arches::TRaX::run_sim_trax(sim_config);
	}
	else if(sim_config.get_string("arch_name") == "STRaTA-RT")
	{
		Arches::STRaTART::run_sim_strata_rt(sim_config);
	}
	else if (sim_config.get_string("arch_name") == "Dual-Streaming")
	{
		Arches::DualStreaming::run_sim_dual_streaming(sim_config);
	}
	
	return 0;
}