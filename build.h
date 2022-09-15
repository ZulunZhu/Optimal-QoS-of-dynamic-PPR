
#ifndef BUILD_H
#define BUILD_H

#include "algo.h"
#include "graph.h"
#include "heap.h"
#include "config.h"

#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/utility.hpp>
#include <sys/sysinfo.h>

inline size_t get_ram_size(){
    struct sysinfo si;
    sysinfo (&si);
    return si.totalram;
}

inline string get_hub_fwd_idx_file_name(){
    string prefix = config.prefix + FILESEP + config.graph_alias+FILESEP;
    prefix += config.graph_alias + ".eps-" + to_str(config.epsilon);
    // prefix += ".space-1";
    prefix += ".space-" + to_str(config.hub_space_consum);

    string suffix;

    suffix += ".compress.fwd.idx";
    string file_name = prefix + suffix;
    return file_name;
}

inline string get_hub_fwd_idx_info_file_name(){
    string idx_file = get_hub_fwd_idx_file_name();
    return replace(idx_file, "fwd.idx", "fwd.info");
}

inline string get_hub_bwd_idx_file_name(){
    string idx_file = get_hub_fwd_idx_file_name();
    return replace(idx_file, "fwd.idx", "bwd.idx");
}

inline void deserialize_hub_fwd_idx(){
    string file_name = get_hub_fwd_idx_file_name();
    assert_file_exist("index file", file_name);
    std::ifstream ifs(file_name);
    boost::archive::binary_iarchive ia(ifs);
    ia >> hub_fwd_idx;
    
    string rwn_file_name = get_hub_fwd_idx_file_name()+".rwn";
    assert_file_exist("rwn file", rwn_file_name);
    std::ifstream ofs_rwn(rwn_file_name);
    boost::archive::binary_iarchive ia_rwn(ofs_rwn);

    ia_rwn >> hub_sample_number;


    string info_file = get_hub_fwd_idx_info_file_name();
    assert_file_exist("info file", info_file);
    std::ifstream info_ofs(info_file);
    boost::archive::binary_iarchive info_ia(info_ofs);
    info_ia >> hub_fwd_idx_cp_pointers;
}

inline void deserialize_hub_bwd_idx(){
    string file_name = get_hub_bwd_idx_file_name();
    // assert_file_exist("index file", file_name);
    if (!exists_test(file_name)) {
        cerr << "index file " << file_name << " not find " << endl;
        return;
    }
    std::ifstream ifs(file_name);
    boost::archive::binary_iarchive ia(ifs);
    ia >> hub_bwd_idx;
}

void load_hubppr_oracle(const Graph& graph){
    deserialize_hub_fwd_idx();

    // fwd_idx_size.initialize(graph.n);
    hub_fwd_idx_ptrs.resize(graph.n);
    hub_fwd_idx_size.resize(graph.n);
    std::fill(hub_fwd_idx_size.begin(), hub_fwd_idx_size.end(), 0);
    hub_fwd_idx_size_k.initialize(graph.n);


    for(auto &ptrs: hub_fwd_idx_cp_pointers){
        int node = ptrs.first;
        int size=0;

        unsigned long long ptr = ptrs.second[0];
        unsigned long long end_ptr = ptrs.second[ptrs.second.size()-1];
        for(; ptr<end_ptr; ptr+=2){
            size += hub_fwd_idx[ptr+1];
        }

        hub_fwd_idx_ptrs[node] = ptrs.second;

        // fwd_idx_size.insert(node, size);
        hub_fwd_idx_size[node] = size;

        int u = 1 + floor(log( hub_fwd_idx_size[node]*1.0 )/log(2)); //we can pre-compute to avoid reduplicate computation
        int k = pow(2, u-1)-1;
        hub_fwd_idx_size_k.insert(node, k);
    }

    hub_fwd_idx_cp_pointers.clear();

    INFO(hub_fwd_idx_size.size());

    deserialize_hub_bwd_idx();
    INFO(hub_bwd_idx.size());
}

inline string get_exact_topk_ppr_file(){
    if(!boost::algorithm::ends_with(config.exact_pprs_folder, FILESEP))
        config.exact_pprs_folder += FILESEP;
    return config.exact_pprs_folder+config.graph_alias+".topk.pprs";
}

inline void save_exact_topk_ppr(){
    string filename = get_exact_topk_ppr_file();
    std::ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
    oa << exact_topk_pprs;
}

inline void load_exact_topk_ppr(){
    string filename = get_exact_topk_ppr_file();
    if(!exists_test(filename)){
        INFO("No exact topk ppr file", filename);
        return;
    }
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
    ia >> exact_topk_pprs;

    INFO(exact_topk_pprs.size());
}

inline void load_ppr_result( vector<vector<PPR_Result>> & ppr_matrix, string file_path){
	int query_num;
	FILE *fin = fopen(file_path.c_str(), "r");
	fscanf(fin, "%d", &query_num);
	INFO(query_num);
	if(query_num>config.check_size){
		query_num=config.check_size;
	}
	ppr_matrix = vector<vector<PPR_Result>>(query_num, vector<PPR_Result>());
	for(long i = 0; i < query_num;  i++){
        int _ranking, _node, value_num;
		double _ppr_result;
		fscanf(fin, "%d", &value_num);
		for(long j = 0;  j < value_num; j++){
			fscanf(fin, "%d%d%lf", &_ranking, &_node, &_ppr_result);
            struct PPR_Result temp={
				_ranking,
				_node,
				_ppr_result
			};
			ppr_matrix[i].push_back(temp);
		}
	}
}

inline void calc_accuracy( vector<vector<PPR_Result>> & algo_ppr_matrix, vector<vector<PPR_Result>> & exact_ppr_matrix,
ofstream &outputfile, int k){
	int query_num=algo_ppr_matrix.size();
	double NDCG=0;
	double rela_err=0;
	double ab_err=0;
    double precision=0;
    int fail_number=0;
	for(int i=0; i<query_num; i++){
		//cerr<<"\r"<<i;
		vector<PPR_Result> algo_ppr_array;
		if(algo_ppr_matrix[i].size()<k)
			algo_ppr_array=algo_ppr_matrix[i];
		else
			algo_ppr_array=vector<PPR_Result> (algo_ppr_matrix[i].begin(),algo_ppr_matrix[i].begin()+k);
		vector<PPR_Result> exact_ppr_array=exact_ppr_matrix[i];
		sort(algo_ppr_array.begin(), algo_ppr_array.end(), 
				[](struct PPR_Result const& l, struct PPR_Result const& r){return l.node < r.node;});
		sort(exact_ppr_array.begin(), exact_ppr_array.end(), 
				[](struct PPR_Result const& l, struct PPR_Result const& r){return l.node < r.node;});
		int node_num=algo_ppr_array.size();
		int index=0;
		double IDCG=0;
		double DCG=0;
		double rela_inacc=0;
		double ab_inacc=0;
        double inpreci_sum=0;
		for(int j=0; j<node_num; j++){
			struct PPR_Result algo_ppr = algo_ppr_array[j];
			for(int x=index; x<exact_ppr_array.size(); x++){
				if(algo_ppr.node==exact_ppr_array[x].node){
                    if(exact_ppr_array[x].ranking>=k){
                        inpreci_sum++;
                        //cout<<exact_ppr_array[x].ranking<<"\t"<<exact_ppr_array[x].ppr_value<<"\t"<<endl;
                    }
					struct PPR_Result exact_ppr=exact_ppr_array[x];
					IDCG=IDCG+(pow(2,exact_ppr_matrix[i][algo_ppr.ranking].ppr_value)-1)/(log(algo_ppr.ranking+2)/log(2));
					DCG=DCG+(pow(2,exact_ppr.ppr_value)-1)/(log(algo_ppr.ranking+2)/log(2));
                    double rela = abs(algo_ppr.ppr_value-exact_ppr.ppr_value)/exact_ppr.ppr_value;
					rela_inacc+=rela;
					ab_inacc+=abs(algo_ppr.ppr_value-exact_ppr.ppr_value);
                    if(rela>=config.epsilon&&exact_ppr.ppr_value>=1.0/config.graph_n){
                        fail_number++;
                        //INFO(rela);
                        //INFO(exact_ppr.ppr_value);
                    }
					index=x+1;
					break;
				}
			}
		}
        //cout<<node_num<<"\t"<<inpreci_sum<<endl;
		rela_err+=rela_inacc/query_num/k;
		ab_err+=ab_inacc/query_num/k;
		NDCG+=DCG/IDCG/query_num;
        precision+=(k-inpreci_sum)/query_num/k;
	}
	//cout<<endl;
	outputfile<<rela_err<<"\t"<<ab_err<<"\t"<<NDCG<<"\t"<<precision<<"\t"<<endl;
    INFO(algo_ppr_matrix[0].size(),exact_ppr_matrix[0].size());
	INFO(rela_err, ab_err, NDCG, precision, fail_number);
}

inline string get_idx_file_name(){
    string file_name;
	if(config.with_baton==true){
		if(!config.opt)
			file_name = config.graph_location+"randwalks_baton.idx";
		else
			file_name = config.graph_location+"randwalks_baton.opt.idx";
	}else{
		if(!config.opt)
			file_name = config.graph_location+"randwalks_fora.idx";
		else
			file_name = config.graph_location+"randwalks_fora.opt.idx";
	}
    if(config.alter_idx==true){
		file_name = config.graph_location+"randwalks_vldb2010.idx";
	}
    
    return file_name;
}

inline string get_idx_info_name(){
    string file_name;
    if(config.with_baton==true){
		if(!config.opt)
			file_name = config.graph_location+"randwalks_baton.info";
		else
			file_name = config.graph_location+"randwalks_baton.opt.info";
	}else{
		if(!config.opt)
			file_name = config.graph_location+"randwalks_fora.info";
		else
			file_name = config.graph_location+"randwalks_fora.opt.info";
	}
    
    return file_name;   
}

inline void deserialize_idx(){
    string file_name = get_idx_file_name();
    assert_file_exist("index file", file_name);
    std::ifstream ifs(file_name);
    boost::archive::binary_iarchive ia(ifs);
    if(config.alter_idx == 0)
        ia >> rw_idx;
    else 
        ia >> rw_idx_alter;

    file_name = get_idx_info_name();
    assert_file_exist("index file", file_name);
    std::ifstream info_ifs(file_name);
    boost::archive::binary_iarchive info_ia(info_ifs);
    info_ia >> rw_idx_info;
}

inline void deserialize_idx_all(){
	{
	config.with_baton=true;
    string file_name = get_idx_file_name();
    assert_file_exist("index file", file_name);
    std::ifstream ifs(file_name);
    boost::archive::binary_iarchive ia(ifs);
    ia >> rw_idx_baton;

    file_name = get_idx_info_name();
    assert_file_exist("index file", file_name);
    std::ifstream info_ifs(file_name);
    boost::archive::binary_iarchive info_ia(info_ifs);
    info_ia >> rw_idx_info_baton;
	}
	
	{
	config.with_baton=false;
    string file_name = get_idx_file_name();
    assert_file_exist("index file", file_name);
    std::ifstream ifs(file_name);
    boost::archive::binary_iarchive ia(ifs);
    ia >> rw_idx_fora;

    file_name = get_idx_info_name();
    assert_file_exist("index file", file_name);
    std::ifstream info_ifs(file_name);
    boost::archive::binary_iarchive info_ia(info_ifs);
    info_ia >> rw_idx_info_fora;
	}
}


inline void serialize_idx(){
    std::ofstream ofs(get_idx_file_name());
    boost::archive::binary_oarchive oa(ofs);
    if(config.alter_idx == 0)
        oa << rw_idx;
    else
        oa << rw_idx_alter;
    std::ofstream info_ofs(get_idx_info_name());
    boost::archive::binary_oarchive info_oa(info_ofs);
    info_oa << rw_idx_info;
}

inline void serialize_idx_alt(){
    std::ofstream ofs(get_idx_file_name());
    boost::archive::binary_oarchive oa(ofs);
    oa << rw_idx_alter;

    std::ofstream info_ofs(get_idx_info_name());
    boost::archive::binary_oarchive info_oa(info_ofs);
    info_oa << rw_idx_info;
}


void single_build(const Graph& graph, int start, int end, vector<int>& rw_data, unordered_map<int, pair<unsigned long long, unsigned long> >& rw_info_map, int core_id){
    unsigned long num_rw;
    for(int v=start; v<end; v++){
        num_rw = ceil(graph.g[v].size()*config.rmax*config.omega);
        rw_info_map[v] = MP(rw_data.size(), num_rw);
        for(unsigned long i=0; i<num_rw; i++){
            int des = random_walk_thd(v, graph, core_id);
            rw_data.push_back(des);
        }
    }
}

void multi_build(const Graph& graph){
    INFO("multithread building...");
    fora_setting(graph.n, graph.m);

    // rw_idx = RwIdx( graph.n, vector<int>() );
    rw_idx_info.resize(graph.n);

    unsigned NUM_CORES = std::thread::hardware_concurrency();
    assert(NUM_CORES >= 2);

    INFO(NUM_CORES);

    unsigned long long rw_max_size = graph.m*config.rmax*config.omega;
    // INFO(rw_max_size, rw_idx.max_size());

    rw_idx.reserve(rw_max_size);

    vector< vector<int> > vec_rw(NUM_CORES+1);
    vector< unordered_map<int, pair<unsigned long long, unsigned long> > > vec_rw_info(NUM_CORES+1);
    std::vector< std::future<void> > futures(NUM_CORES+1);

    int num_node_per_core = graph.n/(NUM_CORES+1);
    int start=0;
    int end=0;

    {
        INFO("rand-walking...");
        Timer tm(1);
        for(int core_id=0; core_id<NUM_CORES+1; core_id++){
            end = start + num_node_per_core;
            if(core_id==NUM_CORES)
                end = graph.n;
            
            vec_rw[core_id].reserve(rw_max_size/NUM_CORES);
            futures[core_id] = std::async( std::launch::async, single_build, std::ref(graph), start, end, std::ref(vec_rw[core_id]), std::ref(vec_rw_info[core_id]), core_id );
            start = end;
        }
        std::for_each( futures.begin(), futures.end(), std::mem_fn(&std::future<void>::wait));
    }

    {
        INFO("merging...");
        Timer tm(2);
        start=0;
        end=0;
        for(int core_id=0; core_id<NUM_CORES+1; core_id++){
            end = start + num_node_per_core;
            if(core_id==NUM_CORES)
                end = graph.n;

            rw_idx.insert( rw_idx.end(), vec_rw[core_id].begin(), vec_rw[core_id].end() );

            for(int v=start; v<end; v++){
                unsigned long long p = vec_rw_info[core_id][v].first;
                unsigned long num_rw = vec_rw_info[core_id][v].second;
                rw_idx_info[v] = MP( p + rw_idx.size()-vec_rw[core_id].size(), num_rw);
            }
            start = end;
        }
    }

    {
        INFO("materializing...");
        INFO(rw_idx.size(), rw_idx_info.size());
        Timer tm(3);
        serialize_idx(); //serialize the idx to disk
    }

    cout << "Memory usage (MB):" << get_proc_memory()/1000.0 << endl << endl;
}

void build(const Graph& graph){
    // size_t space = get_ram_size();
    // size_t estimated_space = sizeof(RwIdx) + graph.n *( sizeof(vector<int>) + config.num_rw*sizeof(int) );

    // if(estimated_space > space) //if estimated raw space overflows system maximum raw space, reset number of rand-walks
    //     config.num_rw = space * config.num_rw / estimated_space;
    
    fora_setting(graph.n, graph.m);

    // rw_idx = RwIdx( graph.n, vector<int>() );
    rw_idx_info.resize(graph.n);

    unsigned long long rw_max_size = graph.m*config.rmax*config.omega;
	if(config.with_baton == true)
		rw_max_size = graph.m*config.beta/config.alpha;
    // INFO(rw_max_size, rw_idx.max_size());

    rw_idx.reserve(rw_max_size);

    {
        INFO("rand-walking...");
        Timer tm(1);
        unsigned long num_rw;
        for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            num_rw = ceil(graph.g[source].size()*config.rmax*config.omega);
			if(config.with_baton == true)
				num_rw = ceil(graph.g[source].size()*config.beta/config.alpha);
            rw_idx_info[source] = MP(rw_idx.size(), num_rw);
            for(unsigned long i=0; i<num_rw; i++){ //for each node, do some rand-walks
                int destination = random_walk(source, graph);
                // rw_idx[source].push_back(destination);
                rw_idx.push_back(destination);
            }
        }
    }
    {
        INFO("materializing...");
        INFO(rw_idx.size(), rw_idx_info.size());
        Timer tm(2);
        serialize_idx(); //serialize the idx to disk
    }

    cout << "Memory usage (MB):" << get_proc_memory()/1000.0 << endl << endl;
}

void build_alter(const Graph& graph){
    // size_t space = get_ram_size();
    // size_t estimated_space = sizeof(RwIdx) + graph.n *( sizeof(vector<int>) + config.num_rw*sizeof(int) );

    // if(estimated_space > space) //if estimated raw space overflows system maximum raw space, reset number of rand-walks
    //     config.num_rw = space * config.num_rw / estimated_space;
    
    fora_setting(graph.n, graph.m);

    fwd_idx.first.nil = -1;
    fwd_idx.second.nil =-1;
    fwd_idx.first.initialize(graph.n);
    fwd_idx.second.initialize(graph.n);

    // rw_idx = RwIdx( graph.n, vector<int>() );
    rw_idx_info.resize(graph.n);

    unsigned long long rw_max_size = graph.m*config.rmax*config.omega;
	if(config.with_baton == true)
		rw_max_size = graph.m*config.beta/config.alpha;

    long long tuned_index_size =0;
    long long original_index_size =0;

    {
        Timer tm(1);
        unsigned long num_rw;
        if(!config.with_baton){
            for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            
                original_index_size += ceil(graph.g[source].size()*config.rmax*config.omega);
                if(config.opt)
                    num_rw = ceil(graph.g[source].size()*config.rmax*(1-config.alpha)*config.omega);
                else
                    num_rw = ceil(graph.g[source].size()*config.rmax*config.omega);
                rw_idx_info[source] = MP(tuned_index_size, num_rw);
                tuned_index_size += num_rw;
            }
        }else{
            for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            
                original_index_size += ceil(graph.g[source].size()*config.beta/config.alpha);
                if(config.opt)
                    num_rw = ceil(graph.g[source].size()*config.beta*(1-config.alpha)/config.alpha);
                else
                    num_rw = ceil(graph.g[source].size()*config.beta/config.alpha);
                rw_idx_info[source] = MP(tuned_index_size, num_rw);
                tuned_index_size += num_rw;
            }
            
        }
    }
    INFO(tuned_index_size);

    rw_idx.reserve(tuned_index_size);

    {
        INFO("rand-walking...");
        INFO(config.rmax, config.omega, config.rmax*config.omega);
        Timer tm(1);
        for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            for(unsigned long i=0; i<rw_idx_info[source].second; i++){ //for each node, do some rand-walks
                int destination = 0;
                if(config.opt)
                    destination = random_walk_no_zero_hop(source, graph);
                else
                    destination = random_walk(source, graph);
                // rw_idx[source].push_back(destination);
                rw_idx.push_back(destination);
            }
        }
        INFO(original_index_size, tuned_index_size, original_index_size*1.0/tuned_index_size,rw_max_size);
    }

    {
        INFO("materializing...");
        INFO(rw_idx.size(), rw_idx_info.size());
        Timer tm(2);
        serialize_idx(); //serialize the idx to disk
    }

    cout << "Memory usage (MB):" << get_proc_memory()/1000.0 << endl << endl;
}

void build_vldb2010(const Graph& graph){
    // size_t space = get_ram_size();
    // size_t estimated_space = sizeof(RwIdx) + graph.n *( sizeof(vector<int>) + config.num_rw*sizeof(int) );

    // if(estimated_space > space) //if estimated raw space overflows system maximum raw space, reset number of rand-walks
    //     config.num_rw = space * config.num_rw / estimated_space;

    fora_setting(graph.n, graph.m);

    // rw_idx = RwIdx( graph.n, vector<int>() );
    rw_idx_info.resize(graph.n);

    unsigned long long rw_max_size = graph.m*config.rmax*config.omega;
	if(config.with_baton == true)
		rw_max_size = graph.m*config.beta/config.alpha;
    INFO(rw_max_size, rw_idx_alter.max_size());

    rw_idx_alter.reserve(rw_max_size);

    {
        INFO("rand-walking...");
        Timer tm(1);
        unsigned long num_rw;
        for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            num_rw = ceil(graph.g[source].size()*config.rmax*config.omega);
			if(config.with_baton == true)
				num_rw = ceil(graph.g[source].size()*config.beta/config.alpha);
            rw_idx_info[source] = MP(rw_idx_alter.size(), num_rw);
            for(unsigned long i=0; i<num_rw; i++){ //for each node, do some rand-walks
                vector<int> randomWalk = random_walk_vldb2010(source, graph);
                // rw_idx[source].push_back(destination);
                rw_idx_alter.push_back(randomWalk);
            }
        }
    }
    {
        INFO("materializing...");
        INFO(rw_idx_alter.size(), rw_idx_info.size());
        Timer tm(2);
        serialize_idx_alt(); //serialize the idx to disk
    }

    cout << "Memory usage (MB):" << get_proc_memory()/1000.0 << endl << endl;
}

void rebuild_idx(const Graph& graph){
    if(config.no_rebuild)
        return;
	rw_idx_info.clear();
	rw_idx.clear();
	rw_idx_info.resize(graph.n);

    unsigned long long rw_max_size = graph.m*config.rmax*config.omega;
	if(config.with_baton == true)
		rw_max_size = graph.m*config.beta/config.alpha;
    else if(config.opt == true)
        rw_max_size = rw_max_size*(1-config.alpha);
    // INFO(rw_max_size, rw_idx.max_size());
	
    rw_idx.reserve(rw_max_size);

    {
        unsigned long num_rw;
        for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            num_rw = ceil(graph.g[source].size()*config.rmax*config.omega);
			if(config.with_baton == true)
				num_rw = ceil(graph.g[source].size()*config.beta/config.alpha);
            else if(config.opt == true)
                num_rw = num_rw*(1-config.alpha);
            rw_idx_info[source] = MP(rw_idx.size(), num_rw);
            for(unsigned long i=0; i<num_rw; i++){ //for each node, do some rand-walks
                int destination = 0;
                if(config.opt&&config.with_baton!=true)
                    destination = random_walk_no_zero_hop(source, graph);
                else
                    destination = random_walk(source, graph);
                rw_idx.push_back(destination);
            }
        }
		//INFO("end-rand-walking...");
    }
	//INFO(rw_idx.size(), rw_idx_info.size());
}

void rebuild_idx_vldb2010(const Graph& graph, int update_edge_start, int update_edge_end, bool is_insert){
	if(is_insert == true){
        //edge update is insert
        for(int i = 0; i < rw_idx_alter.size(); i++){
            for(int j = 0; j < rw_idx_alter[i].size() - 1; j++){
                if(rw_idx_alter[i][j] == update_edge_start){
                    int k = lrand()%graph.g[update_edge_start].size();
                    if(k == 0){
                        while (rw_idx_alter[i].size() > j + 1)
                        {
                            rw_idx_alter[i].pop_back();
                        }
                        int cur = update_edge_end;
                        unsigned long k;
                        vector<int> result;
                        while (true) {
                            rw_idx_alter[i].push_back(cur);
                            if (drand()) {
                                break;
                            }
                            if (graph.g[cur].size()){
                                k = lrand()%graph.g[cur].size();
                                cur = graph.g[cur][ k ];
                            }
                            else{
                                cur = rw_idx_alter[i][0];
                            }
                        }
                    }
                    break;
                }
            }
        }
    }else{
        //edge update is delete
    }
}

void rebuild_idx_all(const Graph& graph){
	rw_idx_info_fora.clear();
	rw_idx_fora.clear();
	rw_idx_info_fora.resize(graph.n);

    unsigned long long rw_max_size_fora = graph.m*config.rmax*config.omega;
	unsigned long long rw_max_size_baton = graph.m*config.beta/config.alpha;
	
    rw_idx_fora.reserve(rw_max_size_fora);
	
	rw_idx_info_baton.clear();
	rw_idx_baton.clear();
	rw_idx_info_baton.resize(graph.n);
	
    rw_idx_baton.reserve(rw_max_size_baton);

    {
        //INFO("rand-walking...");
		
         unsigned long num_rw_fora;
		 unsigned long num_rw_baton;
        for(int source=0; source<graph.n; source++){ //from each node, do rand-walks
            num_rw_fora = ceil(graph.g[source].size()*config.rmax*config.omega);
			num_rw_baton = ceil(graph.g[source].size()*config.beta/config.alpha);
            rw_idx_info_fora[source] = MP(rw_idx_fora.size(), num_rw_fora);
			rw_idx_info_baton[source] = MP(rw_idx_baton.size(), num_rw_baton);
            for(unsigned long i=0; i<num_rw_fora; i++){ //for each node, do some rand-walks
                int destination = random_walk(source, graph);
                // rw_idx[source].push_back(destination);
                rw_idx_fora.push_back(destination);
            }
			for(unsigned long i=0; i<num_rw_baton; i++){ //for each node, do some rand-walks
                int destination = random_walk(source, graph);
                // rw_idx[source].push_back(destination);
                rw_idx_baton.push_back(destination);
            }
        }
    }
}

void update_idx(const Graph& graph, int source){
	unsigned long num_rw = rw_idx_info[source].second;
	//if(config.with_baton == true)
	num_rw = ceil(graph.g[source].size()*config.beta/config.alpha);
	unsigned long begin_idx = rw_idx_info[source].first;
    // INFO(num_rw);
	for(unsigned long i=0; i<num_rw; i++){ //for each node, do some rand-walks
		unsigned long destination = random_walk(source, graph);
		// rw_idx[source].push_back(destination);
		rw_idx[begin_idx+i]=destination;
	}
    
}

void remove_edge(Graph& graph, int u, int v){
    auto pos = std::find(graph.g[u].begin(), graph.g[u].end(), v);
    if (pos != graph.g[u].end())
    {
        graph.g[u].erase(pos);
    }
    pos = std::find(graph.gr[v].begin(),  graph.gr[v].end(), u);
    if (pos != graph.gr[v].end())
    {
        graph.gr[v].erase(pos);
    }
}

void update_graph(Graph& graph, int u, int v){
    bool is_insert = true;
    for (int next : graph.g[u]) {
        if(next==v){
            is_insert = false;
        }	
    }
    if(is_insert){
        graph.m++;
        graph.g[u].push_back(v);
        graph.gr[v].push_back(u);
    }else{
        graph.m--;
        remove_edge(graph, u, v);
    }
}


void print_idx(const Graph& graph, int source){
	unsigned long num_rw = rw_idx_info[source].second;
	unsigned long begin_idx = rw_idx_info[source].first;
	cout<<"=====source : "<<source<<"=========================="<<endl;
	for(unsigned long i=0; i<num_rw; i++){ //for each node, do some rand-walks
		cout<<i<<" : "<<rw_idx[begin_idx+i]<<endl;
	}
}



#endif
