
#ifndef FORA_QUERY_H
#define FORA_QUERY_H

#include "algo.h"
#include "graph.h"
#include "heap.h"
#include "config.h"
#include "build.h"
#include "constrain_ratio.h"
#include <algorithm>
#include<numeric>

#include <iostream>

#include <cstdio>
//#define CHECK_PPR_VALUES 1
//#define CHECK_TOP_K_PPR 1
#define PRINT_PRECISION_FOR_DIF_K 1
// std::mutex mtx;
class fora_query_topk_with_bound;
struct Query{

    int node_id;
    double init_time; // in seconds
    int operator_type;  // 0: QUERY; 1: UPDATE
    double process_time;  // in microsecs
    Query(int id, double t1, int type, int t2) {
        node_id = id;
        init_time = t1;
        operator_type = type;
        process_time = t2;
    }
};
void load_id_from_list(vector<int> &queries, vector<Query> &list_query){
    for(int i = 0; i<list_query.size();i++){
        queries.push_back(list_query[i].node_id);
    }
}
int calc_hop(Graph graph, int u, int v){
    int hop=1;
    bool find_flag=false;
	
	cout<<u<<"  "<<v<<endl;
	if(u==v)
		return 0;
    
    vector<int> list;
    
    list.push_back(u);
    
    unsigned long i=0;
    while(hop<10){
		i=0;
        vector<int> new_list;
		int length=list.size();
		//cout<<list.size()<<endl;
        while(i<length){
            int p=list[i];
            for( int next : graph.g[p] ){
                if(next==v){
                    return hop;
                }else{
                    new_list.push_back(next);
                }
            }			
			i++;
        }
		
		list=new_list;
        hop++;
    }
    return hop;
}


void montecarlo_query(int v, const Graph& graph){
    Timer timer(MC_QUERY);

    rw_counter.clean();
    ppr.reset_zero_values();

    {
        Timer tm(RONDOM_WALK);
        num_total_rw += config.omega;
        for(unsigned long i=0; i<config.omega; i++){
            int destination = random_walk(v, graph);
            if(!rw_counter.exist(destination))
                rw_counter.insert(destination, 1);
            else
                rw_counter[destination] += 1;
        }
    }

    int node_id;
    for(long i=0; i<rw_counter.occur.m_num; i++){
        node_id = rw_counter.occur[i];
        ppr[node_id] = rw_counter[node_id]*1.0/config.omega;
    }

#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

void montecarlo_query_topk(int v, const Graph& graph){
    Timer timer(0);

    rw_counter.clean();
    ppr.clean();

    {
        Timer tm(RONDOM_WALK);
        num_total_rw += config.omega;
        for(unsigned long i=0; i<config.omega; i++){
            int destination = random_walk(v, graph);
            if(!rw_counter.exist(destination))
                rw_counter.insert(destination, 1);
            else
                rw_counter[destination] += 1;
        }
    }

    int node_id;
    for(long i=0; i<rw_counter.occur.m_num; i++){
        node_id = rw_counter.occur[i];
        if(rw_counter.occur[i]>0)
            ppr.insert( node_id, rw_counter[node_id]*1.0/config.omega );
    }
}

void bippr_query(int v, const Graph& graph){
    Timer timer(BIPPR_QUERY);

    rw_counter.clean();
    ppr.reset_zero_values();

    {
        Timer tm(RONDOM_WALK);
        num_total_rw += config.omega;
        INFO(config.omega);
        for(unsigned long i=0; i<config.omega; i++){
            int destination = random_walk(v, graph); 
            if(!rw_counter.exist(destination))
                rw_counter.insert(destination, 1);
            else
                rw_counter[destination] += 1;
        }
    }

    INFO(config.rmax);
    if(config.rmax < 1.0){
        Timer tm(BWD_LU);
        for(long i=0; i<graph.n; i++){
            reverse_local_update_linear(i, graph);
            // if(backresult.first[v] ==0 && backresult.second.size()==0){
            if( (!bwd_idx.first.exist(v)||0==bwd_idx.first[v]) &&  0==bwd_idx.second.occur.m_num){
                continue;
            }
            ppr[i] += bwd_idx.first[v];
            // for(auto residue: backresult.second){
            for(long j=0; j<bwd_idx.second.occur.m_num; j++){
                // ppr[i]+=counts[residue.first]*1.0/config.omega*residue.second;
                int nodeid = bwd_idx.second.occur[j];
                double residual = bwd_idx.second[nodeid];
                int occur;
                if(!rw_counter.exist(nodeid))
                    occur = 0;
                else
                    occur = rw_counter[nodeid]; 

                ppr[i] += occur*1.0/config.omega*residual;
            }
        }
    }else{
        int node_id;
        for(long i=0; i<rw_counter.occur.m_num; i++){
            node_id = rw_counter.occur[i];
            ppr[node_id] = rw_counter[node_id]*1.0/config.omega;
        }
    }
#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

void bippr_query_topk(int v, const Graph& graph){
    Timer timer(0);

    ppr.clean();
    rw_counter.clean();

    {
        Timer tm(RONDOM_WALK);
        num_total_rw += config.omega;
        for(unsigned long i=0; i<config.omega; i++){
            int destination = random_walk(v, graph);
            if(rw_counter.notexist(destination)){
                rw_counter.insert(destination, 1);
            }
            else{
                rw_counter[destination] += 1;
            }
        }
    }

    if(config.rmax < 1.0){
        Timer tm(BWD_LU);
        for(int i=0; i<graph.n; i++){
            reverse_local_update_linear(i, graph);
            if( (!bwd_idx.first.exist(v)||0==bwd_idx.first[v]) &&  0==bwd_idx.second.occur.m_num){
                continue;
            }

            if( bwd_idx.first.exist(v) && bwd_idx.first[v]>0 )
                ppr.insert(i, bwd_idx.first[v]);

            for(long j=0; j<bwd_idx.second.occur.m_num; j++){
                int nodeid = bwd_idx.second.occur[j];
                double residual = bwd_idx.second[nodeid];
                int occur;
                if(!rw_counter.exist(nodeid)){
                    occur = 0;
                }
                else{
                    occur = rw_counter[nodeid]; 
                }

                if(occur>0){
                    if(!ppr.exist(i)){
                        ppr.insert( i, occur*residual/config.omega );
                    }
                    else{
                        ppr[i] += occur*residual/config.omega;
                    }
                }
            }
        }
    }
    else{
        int node_id;
        for(long i=0; i<rw_counter.occur.m_num; i++){
            node_id = rw_counter.occur[i];
            if(rw_counter[node_id]>0){
                if(!ppr.exist(node_id)){
                    ppr.insert( node_id, rw_counter[node_id]*1.0/config.omega );
                }
                else{
                    ppr[node_id] = rw_counter[node_id]*1.0/config.omega;
                }
            }
        }
    }
}

void hubppr_query(int s, const Graph& graph){
    Timer timer(HUBPPR_QUERY);

    ppr.reset_zero_values();

    {
        Timer tm(RONDOM_WALK);
        fwd_with_hub_oracle(graph, s);
        count_hub_dest();
        INFO("finish fwd work", hub_counter.occur.m_num, rw_counter.occur.m_num);
    }

    {
        Timer tm(BWD_LU);
        for(int t=0; t<graph.n; t++){
            bwd_with_hub_oracle(graph, t);
            // reverse_local_update_linear(t, graph);
            if( (bwd_idx.first.notexist(s) || 0==bwd_idx.first[s]) && 0==bwd_idx.second.occur.m_num ){
                continue;
            }

            if(rw_counter.occur.m_num < bwd_idx.second.occur.m_num){ //iterate on smaller-size list
                for (int i=0; i<rw_counter.occur.m_num; i++) {
                    int node = rw_counter.occur[i];
                    if (bwd_idx.second.exist(node)) {
                        ppr[t] += bwd_idx.second[node]*rw_counter[node];
                    }
                }
            }
            else{
                for (int i=0; i<bwd_idx.second.occur.m_num; i++) {
                    int node = bwd_idx.second.occur[i];
                    if (rw_counter.exist(node)) {
                        ppr[t] += rw_counter[node]*bwd_idx.second[node];
                    }
                }
            }
            ppr[t]=ppr[t]/config.omega;
            if(bwd_idx.first.exist(s))
                ppr[t] += bwd_idx.first[s];
        }
    }

#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

void compute_ppr_with_reserve(){
    ppr.clean();
    int node_id;
    double reserve;
    for(long i=0; i< fwd_idx.first.occur.m_num; i++){
        node_id = fwd_idx.first.occur[i];
        reserve = fwd_idx.first[ node_id ];
        if(reserve)
            ppr.insert(node_id, reserve);
    }
}

bool err_cmp(const pair<int,double> a,const pair<int,double> b){
	return a.second > b.second;
}

void lazy_update_fwdidx(const Graph& graph, double theta){
    if(config.no_rebuild)
        return;
	vector< pair<int,double> > error_idx;
	double rsum=0;
	double errsum=0;
	double inaccsum=0;
	for(long i=0; i<fwd_idx.first.occur.m_num; i++){
        int node_id = fwd_idx.first.occur[i];
        double reserve = fwd_idx.first[ node_id ];
		double residue = fwd_idx.second[ node_id ];
		if(residue*(1-inacc_idx[node_id])>0){
			error_idx.push_back(make_pair(node_id,residue*(1-inacc_idx[node_id])));
			rsum+=residue;
			errsum+=residue*(1-inacc_idx[node_id]);
		}
    }
	sort(error_idx.begin(), error_idx.end(), err_cmp);
	long i=0;
	//double errbound=config.epsilon/graph.n/2/rsum*(1-theta);
	double errbound=config.epsilon/graph.n*(1-theta);
	

	
	while(errsum>errbound){
		//cout<<i<<" : "<<errsum<<"--"<<error_idx[i].second<<endl;;
		update_idx(graph,error_idx[i].first);
		inacc_idx[error_idx[i].first]=1;
		errsum-=error_idx[i].second;
		i++;
	}
	//INFO(i);
}

void lazy_update_fwdidx_one_hop(const Graph& graph, double theta, int v){
	vector< pair<int,double> > error_idx;
	double rsum=0;
	double errsum=0;
	double inaccsum=0;
	for(long i=0; i<fwd_idx.first.occur.m_num; i++){
        int node_id = fwd_idx.first.occur[i];
        double reserve = fwd_idx.first[ node_id ];
		double residue = fwd_idx.second[ node_id ];
		if(residue*(1-inacc_idx[node_id])>0){
			error_idx.push_back(make_pair(node_id,residue*(1-inacc_idx[node_id])));
			rsum+=residue;
			errsum+=residue*(1-inacc_idx[node_id]);
		}
    }
	sort(error_idx.begin(), error_idx.end(), err_cmp);
	long i=0;
	double errbound=config.epsilon/(config.alpha*(1-config.alpha)/graph.g[v].size())*(1-theta);
	

	
	while(errsum>errbound){
		cout<<i<<" : "<<errsum<<"--"<<error_idx[i].second<<endl;;
		update_idx(graph,error_idx[i].first);
		inacc_idx[error_idx[i].first]=1;
		errsum-=error_idx[i].second;
		i++;
	}
	//INFO(i);

}

void compute_ppr_with_fwdidx_hybrid(const Graph& graph, double check_rsum, vector<int> &rw_idx_given, 
vector< pair<unsigned long long, unsigned long> > &rw_idx_info_given){
    ppr.reset_zero_values();
	INFO("compute_ppr_with_fwdidx_hybrid");
    int node_id;
    double reserve;
	double sum=0;
	//INFO(fwd_idx.second.occur.m_num);
    for(long i=0; i< fwd_idx.first.occur.m_num; i++){
        node_id = fwd_idx.first.occur[i];
        reserve = fwd_idx.first[ node_id ];
        ppr[node_id] = reserve;
    }
	
    // INFO("rsum is:", check_rsum);
    if(check_rsum == 0.0)
        return;

    unsigned long long num_random_walk = config.omega*check_rsum;
    //num_total_rw += num_random_walk;

    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk
        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){
            fwd_idx.second.occur.Sort();
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                
                num_total_rw += num_s_rw;
				rw_count += num_s_rw;
                
                //for each source node, get rand walk destinations from previously generated idx or online rand walks
                if(num_s_rw > rw_idx_info_given[source].second){ //if we need more destinations than that in idx, rand walk online
                    for(unsigned long k=0; k<rw_idx_info_given[source].second; k++){
                        int des = rw_idx_given[rw_idx_info_given[source].first + k];
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += rw_idx_info_given[source].second;

                    for(unsigned long j=0; j < num_s_rw-rw_idx_info_given[source].second; j++){ //rand walk online
                        int des = random_walk(source, graph);
                        ppr[des] += ppr_incre;
                    }
                }else{ // using previously generated idx is enough
                    for(unsigned long k=0; k<num_s_rw; k++){
                        int des = rw_idx_given[rw_idx_info_given[source].first + k];
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_s_rw;
                }
            }
        }
        else{ //rand walk online
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                
                num_total_rw += num_s_rw;
				rw_count += num_s_rw;
                for(unsigned long j=0; j<num_s_rw; j++){
                    int des = random_walk(source, graph);
                    ppr[des] += ppr_incre;
                }
            }
        }
    }
}


void compute_ppr_with_fwdidx(const Graph& graph, double check_rsum){
	
	if(config.algo==FORA_AND_BATON){
		if(config.with_baton)
			compute_ppr_with_fwdidx_hybrid(graph, check_rsum, rw_idx_baton, rw_idx_info_baton);
		else
			compute_ppr_with_fwdidx_hybrid(graph, check_rsum, rw_idx_fora, rw_idx_info_fora);
		return;
	}
    ppr.reset_zero_values();

    int node_id;
    double reserve;
	double sum=0;
	//INFO(fwd_idx.second.occur.m_num);
    for(long i=0; i< fwd_idx.first.occur.m_num; i++){
        node_id = fwd_idx.first.occur[i];
        reserve = fwd_idx.first[ node_id ];
        ppr[node_id] = reserve;
    }
	
    // INFO("rsum is:", check_rsum);
    if(check_rsum == 0.0)
        return;

    unsigned long long num_random_walk = config.omega*check_rsum;
    //num_total_rw += num_random_walk;

    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk
        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){
            fwd_idx.second.occur.Sort();
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                
                num_total_rw += num_s_rw;
				rw_count += num_s_rw;
                
                //for each source node, get rand walk destinations from previously generated idx or online rand walks
                if(num_s_rw > rw_idx_info[source].second){ //if we need more destinations than that in idx, rand walk online
                    for(unsigned long k=0; k<rw_idx_info[source].second; k++){
                        int des;
                        if(config.alter_idx == 0)
                            des = rw_idx[rw_idx_info[source].first + k];
                        else
                            des = rw_idx_alter[rw_idx_info[source].first + k].back();
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += rw_idx_info[source].second;

                    for(unsigned long j=0; j < num_s_rw-rw_idx_info[source].second; j++){ //rand walk online
                        int des = random_walk(source, graph);
                        ppr[des] += ppr_incre;
                    }
                }else{ // using previously generated idx is enough
                    for(unsigned long k=0; k<num_s_rw; k++){
                        int des;
                        if(config.alter_idx == 0)
                            des = rw_idx[rw_idx_info[source].first + k];
                        else
                            des = rw_idx_alter[rw_idx_info[source].first + k].back();
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_s_rw;
                }
            }
        }
        else{ //rand walk online
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                
                num_total_rw += num_s_rw;
				rw_count += num_s_rw;
                for(unsigned long j=0; j<num_s_rw; j++){
                    int des = random_walk(source, graph);
                    ppr[des] += ppr_incre;
                }
            }
        }
    }
}

void compute_ppr_with_fwdidx_opt(const Graph& graph, double check_rsum){
    ppr.reset_zero_values();

    int node_id;
    double reserve;
    for(long i=0; i< fwd_idx.first.occur.m_num; i++){
        node_id = fwd_idx.first.occur[i];
        reserve = fwd_idx.first[ node_id ];
        ppr[node_id] = reserve;
    }

    // INFO("rsum is:", check_rsum);
    if(check_rsum == 0.0)
        return;

    check_rsum*=(1-config.alpha);
    unsigned long long num_random_walk = config.omega*check_rsum;
    // INFO(num_random_walk);
    //num_total_rw += num_random_walk;

    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk
        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){
            fwd_idx.second.occur.Sort();
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                //double residual = fwd_idx.second[source];
                if(!fwd_idx.second.exist(source)) continue;
                ppr[source]+=fwd_idx.second[source]*config.alpha;
                double residual = fwd_idx.second[source]*(1-config.alpha);


                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                
                num_total_rw += num_s_rw;
                
                //for each source node, get rand walk destinations from previously generated idx or online rand walks
                if(num_s_rw > rw_idx_info[source].second){ //if we need more destinations than that in idx, rand walk online
                    for(unsigned long k=0; k<rw_idx_info[source].second; k++){
                        int des = rw_idx[rw_idx_info[source].first + k];
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += rw_idx_info[source].second;

                    for(unsigned long j=0; j < num_s_rw-rw_idx_info[source].second; j++){ //rand walk online
                        int des = random_walk_no_zero_hop(source, graph);
                        ppr[des] += ppr_incre;
                    }
                }else{ // using previously generated idx is enough
                    for(unsigned long k=0; k<num_s_rw; k++){
                        int des = rw_idx[rw_idx_info[source].first + k];
                        ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_s_rw;
                }
            }
        }
        else{ //rand walk online
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                if(!fwd_idx.second.exist(source)) continue;
                ppr[source]+=fwd_idx.second[source]*config.alpha;
                double residual = fwd_idx.second[source]*(1-config.alpha);
                unsigned long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                num_total_rw += num_s_rw;
                for(unsigned long j=0; j<num_s_rw; j++){
                    int des = random_walk_no_zero_hop(source, graph);
                    ppr[des] += ppr_incre;
                }
            }
        }
    }
}

void compute_ppr_with_fwdidx_topk_with_bound_hybrid(const Graph& graph, double check_rsum,
vector<int> &rw_idx_given, 
vector< pair<unsigned long long, unsigned long> > &rw_idx_info_given, double theta=1.0){
    compute_ppr_with_reserve();
	INFO("compute_ppr_with_fwdidx_topk_with_bound_hybrid");
    if(check_rsum == 0.0)
        return;

    long num_random_walk = config.omega*check_rsum;
		INFO(config.omega,check_rsum);
    long real_num_rand_walk=0;
	rw_counter.reset_zero_values();
    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk

        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){ //rand walk with previously generated idx
            fwd_idx.second.occur.Sort();
            //for each source node, get rand walk destinations from previously generated idx or online rand walks
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
				//cout<<"source: "<<source<<endl;
                double residual = fwd_idx.second[source];
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
				
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;
								
                double ppr_incre = a_s*check_rsum/num_random_walk;

                real_num_rand_walk += num_s_rw;

                long num_used_idx = 0;
                bool source_cnt_exist = rw_counter.exist(source);
                if( source_cnt_exist ){
                    num_used_idx = rw_counter[source];
					//if(num_used_idx>0)
						//continue;
				}
				/*
				if( graph.g[source].size() == 0 ){
					if(ppr.exist(source))
						ppr[source] += ppr_incre*num_s_rw;
					else
						ppr.insert(source, ppr_incre*num_s_rw);
					if(source==24980){
								cout<<24980<<":\t"<<ppr[source]<<endl;
								cout<<ppr_incre<<":\t"<<num_s_rw<<endl;
					}
					continue;
				}*/
				
				//cout<<"num_used_idx: "<<num_used_idx<<endl;
				num_total_rw += num_s_rw;
				
                long num_remaining_idx = rw_idx_info_given[source].second-num_used_idx;
				//cout<<"num_remaining_idx: "<<num_remaining_idx<<endl;
                if(num_s_rw <= num_remaining_idx){
                    // using previously generated idx is enough
                    long k=0;
                    for(; k<num_remaining_idx; k++){
											rw_count++;
                        if( k < num_s_rw){
                            int des = rw_idx_given[rw_idx_info_given[source].first + k];
                            if(ppr.exist(des))
                                ppr[des] += ppr_incre;
                            else
                                ppr.insert(des, ppr_incre);
                        }else
                            break;
                    }
                    if(source_cnt_exist){
                        rw_counter[source] += k;
                    }
                    else{
                        rw_counter.insert(source, k);
                    }

                    num_hit_idx += k;
					//cout<<"num_hit_idx: "<<num_hit_idx<<endl;
                }else{
                    //we need more destinations than that in idx, rand walk online
                    for(long k=0; k<num_remaining_idx; k++){
                        int des = rw_idx_given[ rw_idx_info_given[source].first + k ];
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_remaining_idx;

                    if(!source_cnt_exist){
                        rw_counter.insert( source, num_remaining_idx );
                    }
                    else{
                        rw_counter[source] += num_remaining_idx;
                    }

                    for(long j=0; j < num_s_rw-num_remaining_idx; j++){ //rand walk online
                        int des = random_walk(source, graph);
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else 
                            ppr[des] += ppr_incre;
                    }
                }
            }
        }
        else{ //rand walk online
			double residue_sum=0;
			/*
			long rw_sum;
			long rw_sum_1=0;
			long rw_sum_2=0;
			double residue_sum_1=0;
			 
			for(long i=0; i<fwd_idx.second.occur.m_num; i++){
				long id =fwd_idx.second.occur[i];
				residue_sum += fwd_idx.second[id];
				rw_sum_1+=ceil(fwd_idx.second[id]/check_rsum*num_random_walk);
			}
			rw_sum = ceil(residue_sum/check_rsum*num_random_walk);
			cout<<"res: "<<residue_sum<<endl;
			cout<<"rw: "<<rw_sum<<endl;
			 */
			
            for(int i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
				//residue_sum_1 += fwd_idx.second[source];
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;
				//rw_sum_2+=num_s_rw;

                real_num_rand_walk += num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                for(long j=0; j<num_s_rw; j++){
                    int des = random_walk(source, graph);
                    if(!ppr.exist(des))
                        ppr.insert(des, ppr_incre);
                    else
                        ppr[des] += ppr_incre;

                }
            }
			/*
			cout<<"rs1: "<<residue_sum_1<<endl;
			cout<<"rw1:"<<rw_sum_1<<endl;
			cout<<"rw2:"<<rw_sum_2<<endl;
			cout<<"real_rw"<<real_num_rand_walk<<endl;
			*/
        }
		/*
		for(long i=0; i<ppr.occur.m_num; i++){
						long id = ppr.occur[i];
						cout<<"id: "<<id<<"  rw_value: "<<ppr[id]<<endl;
		}
		 */
    }

    if(config.delta < threshold)
		if(theta==1.0)
			set_ppr_bounds(graph, check_rsum, real_num_rand_walk);
		else
			set_ppr_bounds_dynamic(graph, check_rsum, real_num_rand_walk, theta);
    else{
        zero_ppr_upper_bound = calculate_lambda(check_rsum,  config.pfail, zero_ppr_upper_bound, real_num_rand_walk, theta);
    }
}

void compute_ppr_with_fwdidx_topk(const Graph& graph, double check_rsum){
    // ppr.clean();
    // // ppr.reset_zero_values();

    // int node_id;
    // double reserve;
    // for(long i=0; i< fwd_idx.first.occur.m_num; i++){
    //     node_id = fwd_idx.first.occur[i];
    //     reserve = fwd_idx.first[ node_id ];
    //     ppr.insert(node_id, reserve);
    //     // ppr[node_id] = reserve;
    // }
    compute_ppr_with_reserve();

    // INFO("rsum is:", check_rsum);
    if(check_rsum == 0.0)
        return;

    check_rsum*= (1-config.alpha);
    unsigned long long num_random_walk = config.omega*check_rsum;
    // INFO(num_random_walk);
    //num_total_rw += num_random_walk;
    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk
        //Timer tm(SOURCE_DIST);
        int source;
        double residual;
        unsigned long num_s_rw;
        double a_s;
        double ppr_incre;
        unsigned long num_used_idx;
        unsigned long num_remaining_idx;
        int des;
        //INFO(num_random_walk, fwd_idx.second.occur.m_num);
        if(config.with_rw_idx){ //rand walk with previously generated idx
            fwd_idx.second.occur.Sort();
            //for each source node, get rand walk destinations from previously generated idx or online rand walks
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                source = fwd_idx.second.occur[i];
                residual = fwd_idx.second[source];
                if(ppr.exist(source)){
                    ppr[source] += residual*config.alpha;
                }else{
                    ppr.insert(source, residual*config.alpha);
                }

                residual*=(1-config.alpha);
                num_s_rw = ceil(residual*config.omega);
                a_s = residual*config.omega/num_s_rw;

                ppr_incre = a_s/config.omega;

                num_total_rw += num_s_rw;

                num_used_idx = rw_counter[source];
                num_remaining_idx = rw_idx_info[source].second;
                
                if(num_s_rw <= num_remaining_idx){
                    // using previously generated idx is enough
                    for(unsigned long k=0; k<num_s_rw; k++){
                        des = rw_idx[ rw_idx_info[source].first+ num_used_idx + k ];
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }

                    rw_counter[source] = num_used_idx + num_s_rw;

                    num_hit_idx += num_s_rw;
                }
                else{
                    //INFO(num_s_rw,num_remaining_idx);
                    //we need more destinations than that in idx, rand walk online
                    for(unsigned long k=0; k<num_remaining_idx; k++){
                        des = rw_idx[ rw_idx_info[source].first + num_used_idx + k ];
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }

                    num_hit_idx += num_remaining_idx;
                    rw_counter[source] = num_used_idx + num_remaining_idx;

                    for(unsigned long j=0; j < num_s_rw-num_remaining_idx; j++){ //rand walk online
                        des = random_walk_no_zero_hop(source, graph);
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }
                }
            }
        }
        else{ //rand walk online
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                source = fwd_idx.second.occur[i];
                residual = fwd_idx.second[source];
                num_s_rw = ceil(residual*config.omega);
                a_s = residual*config.omega/num_s_rw;

                ppr_incre = a_s/config.omega;
                num_total_rw += num_s_rw;

                for(unsigned long j=0; j<num_s_rw; j++){
                    des = random_walk(source, graph);
                    if(!ppr.exist(des))
                        ppr.insert(des, ppr_incre);
                    else
                        ppr[des] += ppr_incre;
                }
            }
        }
    }

}

void compute_ppr_with_fwdidx_topk_with_bound(const Graph& graph, double check_rsum, double theta=1.0){
	if(config.algo==FORA_AND_BATON){
		if(config.with_baton)
			compute_ppr_with_fwdidx_topk_with_bound_hybrid(graph, check_rsum, rw_idx_baton, rw_idx_info_baton, theta);
		else
			compute_ppr_with_fwdidx_topk_with_bound_hybrid(graph, check_rsum, rw_idx_fora, rw_idx_info_fora, theta);
		return;
	}
    compute_ppr_with_reserve();
    if(check_rsum == 0.0)
        return;

    long num_random_walk = config.omega*check_rsum;
    long real_num_rand_walk=0;
	rw_counter.reset_zero_values();
    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk

        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){ //rand walk with previously generated idx
            fwd_idx.second.occur.Sort();
            //for each source node, get rand walk destinations from previously generated idx or online rand walks
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
				//cout<<"source: "<<source<<endl;
                double residual = fwd_idx.second[source];
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
				
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;
				

                double ppr_incre = a_s*check_rsum/num_random_walk;

               
                real_num_rand_walk += num_s_rw;

                long num_used_idx = 0;
                bool source_cnt_exist = rw_counter.exist(source);
                if( source_cnt_exist ){
                    num_used_idx = rw_counter[source];
					//if(num_used_idx>0)
						//continue;
				}
				
				/*
				if( graph.g[source].size() == 0 ){
					if(ppr.exist(source))
						ppr[source] += ppr_incre*num_s_rw;
					else
						ppr.insert(source, ppr_incre*num_s_rw);
					if(source==24980){
								cout<<24980<<":\t"<<ppr[source]<<endl;
								cout<<ppr_incre<<":\t"<<num_s_rw<<endl;
					}
					continue;
				}*/
				
				//cout<<"num_used_idx: "<<num_used_idx<<endl;
				num_total_rw += num_s_rw;
				
                long num_remaining_idx = rw_idx_info[source].second-num_used_idx;
				//cout<<"num_remaining_idx: "<<num_remaining_idx<<endl;
				
                
                if(num_s_rw <= num_remaining_idx){
                    // using previously generated idx is enough
                    long k=0;
                    for(; k<num_remaining_idx; k++){
											rw_count++;
                        if( k < num_s_rw){
                            int des = rw_idx[rw_idx_info[source].first + k];
                            if(ppr.exist(des))
                                ppr[des] += ppr_incre;
                            else
                                ppr.insert(des, ppr_incre);
                        }else
                            break;
                    }
                    if(source_cnt_exist){
                        rw_counter[source] += k;
                    }
                    else{
                        rw_counter.insert(source, k);
                    }

                    num_hit_idx += k;
					//cout<<"num_hit_idx: "<<num_hit_idx<<endl;
                }else{
                    //we need more destinations than that in idx, rand walk online

                    for(long k=0; k<num_remaining_idx; k++){
                        int des = rw_idx[ rw_idx_info[source].first + k ];
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_remaining_idx;

                    if(!source_cnt_exist){
                        rw_counter.insert( source, num_remaining_idx );
                    }
                    else{
                        rw_counter[source] += num_remaining_idx;
                    }

                    for(long j=0; j < num_s_rw-num_remaining_idx; j++){ //rand walk online
                        int des = random_walk(source, graph);
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else 
                            ppr[des] += ppr_incre;
                    }
                }
            }
        }
        else{ //rand walk online
			double residue_sum=0;
			/*
			long rw_sum;
			long rw_sum_1=0;
			long rw_sum_2=0;
			double residue_sum_1=0;
			 
			for(long i=0; i<fwd_idx.second.occur.m_num; i++){
				long id =fwd_idx.second.occur[i];
				residue_sum += fwd_idx.second[id];
				rw_sum_1+=ceil(fwd_idx.second[id]/check_rsum*num_random_walk);
			}
			rw_sum = ceil(residue_sum/check_rsum*num_random_walk);
			cout<<"res: "<<residue_sum<<endl;
			cout<<"rw: "<<rw_sum<<endl;
			 */
			
            for(int i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
				//residue_sum_1 += fwd_idx.second[source];
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;
				//rw_sum_2+=num_s_rw;

                real_num_rand_walk += num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                for(long j=0; j<num_s_rw; j++){
                    int des = random_walk(source, graph);
                    if(!ppr.exist(des))
                        ppr.insert(des, ppr_incre);
                    else
                        ppr[des] += ppr_incre;

                }
            }
			/*
			cout<<"rs1: "<<residue_sum_1<<endl;
			cout<<"rw1:"<<rw_sum_1<<endl;
			cout<<"rw2:"<<rw_sum_2<<endl;
			cout<<"real_rw"<<real_num_rand_walk<<endl;
			*/
        }
		/*
		for(long i=0; i<ppr.occur.m_num; i++){
						long id = ppr.occur[i];
						cout<<"id: "<<id<<"  rw_value: "<<ppr[id]<<endl;
		}
		 */
    }

    if(config.delta < threshold)
		if(theta==1.0)
			set_ppr_bounds(graph, check_rsum, real_num_rand_walk);
		else
			set_ppr_bounds_dynamic(graph, check_rsum, real_num_rand_walk, theta);
    else{
        zero_ppr_upper_bound = calculate_lambda(check_rsum,  config.pfail, zero_ppr_upper_bound, real_num_rand_walk, theta);
    }
}




void compute_ppr_with_fwdidx_topk_with_bound_baton(const Graph& graph, double check_rsum){

    compute_ppr_with_reserve();

    if(check_rsum == 0.0)
        return;

    long num_random_walk = config.omega*check_rsum;
    long real_num_rand_walk=0;
	rw_counter.reset_zero_values();
    {
        Timer timer(RONDOM_WALK); //both rand-walk and source distribution are related with num_random_walk

        //Timer tm(SOURCE_DIST);
        if(config.with_rw_idx){ //rand walk with previously generated idx
            fwd_idx.second.occur.Sort();
			//cout<<fwd_idx.second.occur.m_num<<endl;
            //for each source node, get rand walk destinations from previously generated idx or online rand walks
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
				//cout<<"source: "<<source<<endl;
                double residual = fwd_idx.second[source];
				if(residual==0)
					continue;
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
				
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;

               
                real_num_rand_walk += num_s_rw;

                long num_used_idx = 0;
                bool source_cnt_exist = rw_counter.exist(source);
                if( source_cnt_exist ){
                    num_used_idx = rw_counter[source];
					//if(num_used_idx>0)
						//continue;
				}
				if( graph.g[source].size() == 0 ){
					if(ppr.exist(source))
						ppr[source] += ppr_incre*num_s_rw;
					else
						ppr.insert(source, ppr_incre*num_s_rw);
					continue;
				}
                long num_reuse_idx = 0;
				
				//cout<<"++++++++++++++++"<<endl;
				//cout<<"source: "<<source<<endl;
				
				num_reuse_idx=used_idx[source];

				//cout<<"num_used_idx: "<<num_used_idx<<endl;
				num_total_rw += num_s_rw;
                long num_remaining_idx = rw_idx_info[source].second;
                long num_noreuse_idx = num_s_rw - num_reuse_idx;
				//cout<<"num_remaining_idx: "<<num_remaining_idx<<endl;
				num_hit_idx += num_reuse_idx + num_noreuse_idx;
                //cout<<"ppr_incre"<<ppr_incre<<endl;
				//cout<<num_s_rw<<endl;
				//cout<<"-"<<num_reuse_idx<<"-   -"<<num_noreuse_idx<<"-"<<endl;
                if(num_s_rw <= num_remaining_idx){
					
                    // using previously generated idx is enough
					if(num_noreuse_idx>0){
						long k=0;
						//cout<<num_noreuse_idx<<endl;
						//cout<<num_reuse_idx<<endl;
						for(; k<num_noreuse_idx; k++){
							rw_count++;
							int des = rw_idx[rw_idx_info[source].first + k + num_reuse_idx];
							/*
							if(reuse_idx.exist(des))
								reuse_idx[des] ++;
							else
								reuse_idx.insert(des, 1);
								 */
							reuse_idx_vector[des]++;
						}	
						used_idx[source]=num_s_rw;
					}else{
						long k=0;
						for(; k>num_noreuse_idx; k--){
							rw_count++;
							int des = rw_idx[rw_idx_info[source].first + k + num_reuse_idx - 1];
							reuse_idx_vector[des]--;
						}
						used_idx[source]=num_s_rw;
						
					}
					//cout<<"num_hit_idx: "<<num_hit_idx<<endl;
                }else{
                    //we need more destinations than that in idx, rand walk online
					
					cout<<"num_s_rw: "<<num_s_rw<<endl;
					cout<<"idx: "<<rw_idx_info[source].second<<endl;
					cout<<"source: "<<source<<endl;
					cout<<"dout: "<<graph.g[source].size()<<endl;
					
                    for(long k=0; k<num_remaining_idx; k++){
                        int des = rw_idx[ rw_idx_info[source].first + k ];
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else
                            ppr[des] += ppr_incre;
                    }
                    num_hit_idx += num_remaining_idx;

                    if(!source_cnt_exist){
                        rw_counter.insert( source, num_remaining_idx );
                    }
                    else{
                        rw_counter[source] += num_remaining_idx;
                    }

                    for(long j=0; j < num_s_rw-num_remaining_idx; j++){ //rand walk online
                        int des = random_walk(source, graph);
                        if(!ppr.exist(des))
                            ppr.insert(des, ppr_incre);
                        else 
                            ppr[des] += ppr_incre;
                    }
                }
            }
        }
        else{ //rand walk online
            for(long i=0; i < fwd_idx.second.occur.m_num; i++){
                int source = fwd_idx.second.occur[i];
                double residual = fwd_idx.second[source];
                long num_s_rw = ceil(residual/check_rsum*num_random_walk);
                double a_s = residual/check_rsum*num_random_walk/num_s_rw;

                real_num_rand_walk += num_s_rw;

                double ppr_incre = a_s*check_rsum/num_random_walk;
                for(long j=0; j<num_s_rw; j++){
                    int des = random_walk(source, graph);
                    if(!ppr.exist(des))
                        ppr.insert(des, ppr_incre);
                    else
                        ppr[des] += ppr_incre;

                }
            }
        }
		Timer tm(20);
		double ppr_incre=check_rsum/num_random_walk;
		long length=reuse_idx_vector.size();
		//cout<<reuse_idx.occur.m_num<<endl;
		for(long i=0; i < length; i++){
			if(reuse_idx_vector[i]>0){
				if(ppr.exist(i))
					ppr[i] += ppr_incre*reuse_idx_vector[i];
				else
					ppr.insert(i, ppr_incre*reuse_idx_vector[i]);
			}
		}
		
	
    }

    if(config.delta < threshold)
        set_ppr_bounds(graph, check_rsum, real_num_rand_walk);
    else{
        zero_ppr_upper_bound = calculate_lambda(check_rsum,  config.pfail, zero_ppr_upper_bound, real_num_rand_walk);
    }
}

void resacc_query(int v, const Graph& graph){
    Timer timer(FORA_QUERY);
	//INFO(v);
    double rsum = 0.0;
	double r_max_hop=0.1;
	double r_max_f=0.1;
	int k_hops=2;
	int vert = graph.n;
	double* resacc_residual = new double[vert];
	double* resacc_ppr = new double[vert];
	int* hops_from_source = new int[vert];
	for(int i = 0; i < vert; i++){
		resacc_ppr[i] = 0.0;
		resacc_residual[i] = 0.0;
		hops_from_source[i] = numeric_limits<int>::max();
	}
	resacc_residual[v] = 1.0;
	hops_from_source[v] = 0;
	unordered_set<int> kHopSet;
	unordered_set<int> k1HopLayer;
    {
        Timer timer(FWD_LU);
		k1HopLayer = kHopFWD(v, k_hops, hops_from_source, kHopSet, k1HopLayer, r_max_hop, resacc_ppr, resacc_residual, graph);
		//for(int i = 0; i < vert; i++)
			//rsum+=resacc_residual[i];
			rsum=1;
		r_max_f=config.epsilon * r_max_f / sqrt( (graph.m) * 3.0 * log(2.0 * graph.n) * graph.n);
        forward_local_update_resacc(v, graph, rsum, config.rmax, resacc_residual, resacc_ppr, k1HopLayer); //forward propagation, obtain reserve and residual
    }

    // compute_ppr_with_fwdidx(graph);
    compute_ppr_with_fwdidx(graph, rsum);

#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

double total_rsum = 0.0;
double random_walk_time = 0.0000004;
double random_walk_index_time = random_walk_time/140;
double previous_rmax = 0;

double estimated_random_walk_cost(double rsum, double rmax){
    double estimated_random_walk_cost = 0.0;
    if(!config.with_rw_idx){
        estimated_random_walk_cost = config.omega*rsum*(1-config.alpha)*random_walk_time;
    }else{
        if(rmax >= config.rmax){
            estimated_random_walk_cost = config.omega*rsum*(1-config.alpha)*random_walk_time;
        }else{
            estimated_random_walk_cost = config.omega*rsum*(1-config.alpha)*random_walk_index_time;
        }
    }
    INFO(rmax, config.rmax, estimated_random_walk_cost);
    return estimated_random_walk_cost;
}

void fora_query_basic(int v, const Graph& graph){
    Timer timer(FORA_QUERY);
    std::chrono::steady_clock::time_point startTime;
    startTime = std::chrono::steady_clock::now();
	//INFO(v);
    double rsum = 1.0;
    if(config.balanced){
        static vector<int> forward_from;
        forward_from.clear();
        forward_from.reserve(graph.n);
        forward_from.push_back(v);

        fwd_idx.first.clean();  //reserve
        fwd_idx.second.clean();  //residual
        fwd_idx.second.insert( v, rsum );

        const static double min_delta = 1.0 / graph.n;

        const static double lowest_delta_rmax = config.opt?config.epsilon*sqrt(min_delta/3/graph.m/log(2/config.pfail))/(1-config.alpha):config.epsilon*sqrt(min_delta/3/graph.m/log(2/config.pfail));
        double used_time = 0;
        double rmax = 0;
        rmax = config.rmax*8;
        double random_walk_cost = 0;
        if(graph.g[v].size()>0){
            while(estimated_random_walk_cost(rsum, rmax)> used_time){
                INFO(config.omega*rsum*random_walk_time, used_time);
                std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
                forward_local_update_linear_topk( v, graph, rsum, rmax, lowest_delta_rmax, forward_from ); 
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - startTime).count();
                INFO(rsum);
                used_time +=duration/TIMES_PER_SEC;
                double used_time_this_iteration = duration/TIMES_PER_SEC;
                INFO(used_time_this_iteration);
                rmax /=2;
            }
            rmax*=2;
            INFO("Adpaitve total forward push time: ", used_time);
            INFO(config.rmax, rmax, config.rmax/rmax);
            //count_ratio[config.rmax/rmax]++;
        }else{
            forward_local_update_linear(v, graph, rsum, config.rmax);
        }
    }else{
        Timer timer(FWD_LU);
        forward_local_update_linear(v, graph, rsum, config.rmax); //forward propagation, obtain reserve and residual
    }

    auto duration_query1 = get_duration(startTime);
    qtau1_vector.push_back(duration_query1);
    if(config.opt){
        compute_ppr_with_fwdidx_opt(graph, rsum);
        total_rsum+= rsum*(1-config.alpha);
    }else{
        compute_ppr_with_fwdidx(graph, rsum);
        total_rsum+= rsum*(1-config.alpha);
    }

    auto duration_query2 = get_duration(startTime);
    qtau2_vector.push_back(duration_query2-duration_query1);
#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

void fora_query_dynamic(int v, const Graph& graph, double sigma){
    Timer timer(FORA_QUERY);
    double rsum = 1.0;
	double temp_eps=config.epsilon;
	ASSERT(sigma<1);
	config.epsilon=config.epsilon*(1-sigma);
	config.omega = (2+config.epsilon)*log(2/config.pfail)/config.delta/config.epsilon/config.epsilon;
	//cout<<"111"<<endl;
    {
        Timer timer(FWD_LU);
        forward_local_update_linear(v, graph, rsum, config.rmax); //forward propagation, obtain reserve and residual
    }
	//cout<<"222"<<endl;
	config.epsilon=temp_eps;
    // compute_ppr_with_fwdidx(graph);
    compute_ppr_with_fwdidx(graph, rsum);

#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif
}

void fora_query_lazy_dynamic(int v, const Graph& graph, double theta){
	
    double rsum = 1.0;
	double temp_eps=config.epsilon;
    double temp_omg=config.omega;
    config.epsilon=config.epsilon*theta;
    config.omega = (2+config.epsilon)*log(2/config.pfail)/config.delta/config.epsilon/config.epsilon;
    


	std::chrono::steady_clock::time_point startTime;
    startTime = std::chrono::steady_clock::now();
    {
		Timer timer1(FORA_QUERY);
        Timer timer(FWD_LU);
        forward_local_update_linear(v, graph, rsum, config.rmax); //forward propagation, obtain reserve and residual
    }
    auto duration_query1 = get_duration(startTime);
    qtau1_vector.push_back(duration_query1);
    // qtau1 = max(duration_query1, qtau1);
    // INFO(duration_query1);
	config.epsilon=temp_eps;
	config.omega=temp_omg;
	{
		Timer timer(REBUILD_INDEX);	
		lazy_update_fwdidx(graph, theta);

	}
	auto duration_query2 = get_duration(startTime);
    qtau2_vector.push_back(duration_query2-duration_query1);
    // qtau2 = max(duration_query2-duration_query1, qtau2);
    if(config.show_each<10){
        cout<<" Random walk time: "<<(duration_query2-duration_query1)<<endl;
    }
	Timer timer(FORA_QUERY);
    // compute_ppr_with_fwdidx(graph);
	
    compute_ppr_with_fwdidx(graph, rsum);
    auto duration_query3 = get_duration(startTime);
    qtau3_vector.push_back(duration_query3-duration_query2);
    // qtau3 = max(duration_query3-duration_query2, qtau3);
    // INFO(duration_query3-duration_query2);
#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif

}

void one_hop_query_dynamic(int v, const Graph& graph, double theta, bool is_lazy=false){
	
    double rsum = 1.0;
	double temp_eps=config.epsilon;
	config.epsilon=config.epsilon*theta;
	config.omega = (2+config.epsilon)*log(2/config.pfail)/(config.alpha*(1-config.alpha)/graph.g[v].size())/config.epsilon/config.epsilon;
	
    {
		Timer timer1(FORA_QUERY);
        Timer timer(FWD_LU);
        forward_local_update_linear(v, graph, rsum, config.rmax); //forward propagation, obtain reserve and residual
    }
	config.epsilon=temp_eps;
	
	if(is_lazy){
		Timer timer(REBUILD_INDEX);	
		lazy_update_fwdidx_one_hop(graph, theta, v);
	}
	
	Timer timer(FORA_QUERY);
    // compute_ppr_with_fwdidx(graph);
	
    compute_ppr_with_fwdidx(graph, rsum);

#ifdef CHECK_PPR_VALUES
    display_ppr();
#endif

}

void fora_query_topk_new(int v, const Graph& graph ){
    Timer timer(0);
    const static double min_delta = 1.0 / graph.n;
    if(config.k ==0) config.k = 500;
    const static double init_delta = 1.0/config.k/10;//(1.0-config.ppr_decay_alpha)/pow(500, config.ppr_decay_alpha) / pow(graph.n, 1-config.ppr_decay_alpha);
    const static double new_pfail = 1.0 / graph.n / graph.n;
    
    config.pfail = new_pfail;  // log(1/pfail) -> log(1*n/pfail)
    config.delta = init_delta;

    const static double lowest_delta_rmax = config.epsilon*sqrt(min_delta/3/graph.m/log(2/new_pfail));

    double rsum = 1.0;

    static vector<int> forward_from;
    forward_from.clear();
    forward_from.reserve(graph.n);
    forward_from.push_back(v);

    fwd_idx.first.clean();  //reserve
    fwd_idx.second.clean();  //residual
    fwd_idx.second.insert( v, rsum );

    

    if(config.with_rw_idx)
        rw_counter.reset_zero_values(); //store the pointers of each node's used rand-walk idxs 

    // for delta: try value from 1/4 to 1/n
    while( config.delta >= min_delta ){
        fora_topk_setting(graph.n, graph.m);
        num_iter_topk++;
        {
            Timer timer(FWD_LU);
            //INFO(config.rmax, graph.m*config.rmax, config.omega);
            if(graph.g[v].size()==0){
                rsum = 0.0;
                fwd_idx.first.insert(v, 1);
                compute_ppr_with_reserve();
                return;
            }else{
                forward_local_update_linear_topk( v, graph, rsum, config.rmax, lowest_delta_rmax, forward_from ); //forward propagation, obtain reserve and residual
            }

        }

        //i_destination_count.clean();
        //compute_ppr_with_fwdidx_new(graph, rsum);
        //compute_ppr_with_fwdidx_topk(graph, rsum);
        compute_ppr_with_fwdidx_topk(graph, rsum);

        
        {
            double kth_ppr_score = kth_ppr();

            //topk_ppr();

            //double kth_ppr_score = topk_pprs[config.k-1].second;
            if( kth_ppr_score >= (1+config.epsilon)*config.delta || config.delta <= min_delta ){  // once k-th ppr value in top-k list >= (1+epsilon)*delta, terminate
                INFO(kth_ppr_score, config.delta, rsum);
                break;
            }
            else{
                /*int j=0;
                for(; j<config.k; j++){
                    //INFO(topk_pprs[j].second, (1+config.epsilon)*config.delta);
                    if(topk_pprs[j].second<(1+config.epsilon)*config.delta)
                        break;
                }
                INFO("Our current accurate top-j", j);*/
                config.delta = max( min_delta, config.delta/4.0 );  // otherwise, reduce delta to delta/4
            }
        }
    }
}

void fora_query_topk_with_bound(int v, const Graph& graph){

    Timer tm_0(0);

    const static double min_delta = 1.0 / graph.n;
    const static double init_delta = 1.0 / config.k;
    threshold = (1.0-config.ppr_decay_alpha)/pow(500, config.ppr_decay_alpha) / pow(graph.n, 1-config.ppr_decay_alpha);
	//cout<<"threshold: "<<threshold<<endl;

    const static double new_pfail = 1.0 / graph.n / graph.n/log(graph.n);

    config.pfail = new_pfail;  // log(1/pfail) -> log(1*n/pfail)
    config.delta = init_delta;
    
    double lowest_delta_rmax = config.epsilon*sqrt(min_delta/3/graph.m/log(2/new_pfail));
	if(config.with_baton == true)
		lowest_delta_rmax = config.beta/(config.omega*config.alpha);

    double rsum = 1.0;

    static vector<int> forward_from;
    forward_from.clear();
    forward_from.reserve(graph.n);
    forward_from.push_back(v);

    fwd_idx.first.clean();  //reserve
    fwd_idx.second.clean();  //residual
    fwd_idx.second.insert( v, rsum );

    zero_ppr_upper_bound = 1.0;

    if(config.with_rw_idx)
        rw_counter.reset_zero_values(); //store the pointers of each node's used rand-walk idxs 

    // for delta: try value from 1/4 to 1/n
    int iteration = 0;
    upper_bounds.reset_one_values();
    lower_bounds.reset_zero_values();
	
	ppr_rw.clean();
	//reuse_idx.clean();
	reuse_idx_vector.reserve(graph.n);
	reuse_idx_vector.assign(graph.n, 0);
	used_idx.assign(graph.n, 0);
	current_cycle=1000;
    while( config.delta >= min_delta ){
		cycle+=1;
		current_cycle++;
		//Timer tm(current_cycle);
		//cout<<"++++++++++++++++++++++++++++++++++"<<endl;
        fora_setting(graph.n, graph.m);
        num_iter_topk++;

        {
            Timer timer(FWD_LU);
            forward_local_update_linear_topk( v, graph, rsum, config.rmax, lowest_delta_rmax, forward_from ); //forward propagation, obtain reserve and residual
        }
		if(config.reuse == true)
			compute_ppr_with_fwdidx_topk_with_bound_baton(graph, rsum);
		else
			compute_ppr_with_fwdidx_topk_with_bound(graph, rsum);
        if(if_stop() || config.delta <= min_delta){
            break;
        }else
            config.delta = max( min_delta, config.delta/2.0 );  // otherwise, reduce delta to delta/2
    }
	cout<<"Id: "<<v<<"  Delta: "<<config.delta<<endl;
    INFO(config.rmax);
	//display_setting();
}

iMap<int> updated_pprs;
void hubppr_query_topk_martingale(int s, const Graph& graph) {
    unsigned long long the_omega = 2*config.rmax*log(2*config.k/config.pfail)/config.epsilon/config.epsilon/config.delta;
    static double bwd_cost_div = 1.0*graph.m/graph.n/config.alpha;

    static double min_ppr = 1.0/graph.n;
    static double new_pfail = config.pfail/2.0/graph.n/log2(1.0*graph.n*config.alpha*graph.n*graph.n);
    static double pfail_star = log(new_pfail/2);

    static std::vector<bool> target_flag(graph.n);
    static std::vector<double> m_omega(graph.n);
    static vector<vector<int>> node_targets(graph.n);
    static double cur_rmax=1;

    // rw_counter.clean();
    for(int t=0; t<graph.n; t++){
        map_lower_bounds[t].second = 0;//min_ppr;
        upper_bounds[t] = 1.0;
        target_flag[t] = true;
        m_omega[t]=0;
    }

    int num_iter = 1;
    int target_size=graph.n;
    if(cur_rmax>config.rmax){
        cur_rmax=config.rmax;
        for(int t=0; t<graph.n; t++){
            if(target_flag[t]==false)
                continue;
            reverse_local_update_topk(s, t, reserve_maps[t], cur_rmax, residual_maps[t], graph);
            for(const auto &p: residual_maps[t]){
                node_targets[p.first].push_back(t);
            }
        }
    }
    while( target_size > config.k && num_iter<=64 ){ //2^num_iter <= 2^64 since 2^64 is the largest unsigned integer here
        unsigned long long num_rw = pow(2, num_iter);
        rw_counter.clean();
        generate_accumulated_fwd_randwalk(s, graph, num_rw);
        updated_pprs.clean();
        // update m_omega
        {
            for(int x=0; x<rw_counter.occur.m_num; x++){
                int node = rw_counter.occur[x];
                for(const int t: node_targets[node]){
                    if(target_flag[t]==false)
                        continue;
                    m_omega[t] += rw_counter[node]*residual_maps[t][node];
                    if(!updated_pprs.exist(t))
                        updated_pprs.insert(t, 1);
                }
            }
        }

        double b = (2*num_rw-1)*pow(cur_rmax/2.0, 2);
        double lambda = sqrt(pow(cur_rmax*pfail_star/3, 2) - 2*b*pfail_star) - cur_rmax*pfail_star/3;
        {
            for(int i=0; i<updated_pprs.occur.m_num; i++){
                int t = updated_pprs.occur[i];
                if( target_flag[t]==false )
                    continue;

                double reserve = 0;
                if(reserve_maps[t].find(s)!=reserve_maps[t].end()){
                    reserve = reserve_maps[t][s];
                }
                set_martingale_bound(lambda, 2*num_rw-1, t, reserve, cur_rmax, pfail_star, min_ppr, m_omega[t]);
            }
        }

        topk_pprs.clear();
        topk_pprs.resize(config.k);
        partial_sort_copy(map_lower_bounds.begin(), map_lower_bounds.end(), topk_pprs.begin(), topk_pprs.end(), 
            [](pair<int, double> const& l, pair<int, double> const& r){return l.second > r.second;});

        double k_bound = topk_pprs[config.k-1].second;
        if( k_bound*(1+config.epsilon) >= upper_bounds[topk_pprs[config.k-1].first] || (num_rw >= the_omega && cur_rmax <= config.rmax) ){
            break;
        }

        for(int t=0; t<graph.n; t++){
            if(target_flag[t]==true && upper_bounds[t] <= k_bound){
                target_flag[t] = false;
                target_size--;
            }
        }
        num_iter++;
    }
}

void get_topk_dynamic(int v, Graph &graph, double theta, bool if_lazy){
    min_delta = 1.0 / graph.n;
    const static double init_delta = 1.0 / config.k;
    threshold = (1.0-config.ppr_decay_alpha)/pow(500, config.ppr_decay_alpha) / pow(graph.n, 1-config.ppr_decay_alpha);
	//cout<<"threshold: "<<threshold<<endl;

    const static double new_pfail = 1.0 / graph.n / graph.n/log(graph.n);

    config.pfail = new_pfail;  // log(1/pfail) -> log(1*n/pfail)
    config.delta = init_delta;

    double lowest_delta_rmax = config.epsilon*sqrt(min_delta/3/graph.m/log(2/new_pfail));
	if(config.with_baton == true)
		lowest_delta_rmax = config.beta/(config.omega*config.alpha);

    double rsum = 1.0;

    static vector<int> forward_from;
    forward_from.clear();
    forward_from.reserve(graph.n);
    forward_from.push_back(v);

    fwd_idx.first.clean();  //reserve
    fwd_idx.second.clean();  //residual
    fwd_idx.second.insert( v, rsum );

    zero_ppr_upper_bound = 1.0;

    if(config.with_rw_idx)
        rw_counter.reset_zero_values(); //store the pointers of each node's used rand-walk idxs 

    // for delta: try value from 1/4 to 1/n
    int iteration = 0;
    upper_bounds.reset_one_values();
    lower_bounds.reset_zero_values();
	
	ppr_rw.clean();
	//reuse_idx.clean();
	reuse_idx_vector.reserve(graph.n);
	reuse_idx_vector.assign(graph.n, 0);
	used_idx.assign(graph.n, 0);
	current_cycle=1000;
    while( config.delta >= min_delta ){
		cycle+=1;
		current_cycle++;
		//Timer tm(current_cycle);
		//cout<<"++++++++++++++++++++++++++++++++++"<<endl;
        fora_setting(graph.n, graph.m);
        num_iter_topk++;

        {
            Timer timer(FWD_LU);
            forward_local_update_linear_topk( v, graph, rsum, config.rmax, lowest_delta_rmax, forward_from ); //forward propagation, obtain reserve and residual
        }
		if(config.algo == LAZYUP){
			Timer timer(REBUILD_INDEX);	
			lazy_update_fwdidx(graph, theta);			
		}
		compute_ppr_with_fwdidx_topk_with_bound(graph, rsum, theta);
        if(if_stop() || config.delta <= min_delta){
            break;
        }else
            config.delta = max( min_delta, config.delta/config.n );  // otherwise, reduce delta to delta/2
    }
	//cout<<"Id: "<<v<<"  Delta: "<<config.delta<<endl;
	topk_ppr();
}

void get_topk_dynamic_new(int v, Graph &graph, double theta, bool if_lazy){
    const static double min_delta = 1.0 / graph.n;
    
    if(config.k ==0) config.k = 500;
    const static double init_delta = 1.0/config.k/10;//(1.0-config.ppr_decay_alpha)/pow(500, config.ppr_decay_alpha) / pow(graph.n, 1-config.ppr_decay_alpha);
    const static double new_pfail = 1.0 / graph.n / graph.n;

    config.pfail = new_pfail;  // log(1/pfail) -> log(1*n/pfail)
    config.delta = init_delta;

    double lowest_delta_rmax = config.epsilon*sqrt(min_delta/3/graph.m/log(2/new_pfail));
    if(config.with_baton == true)
		lowest_delta_rmax = config.beta/(config.omega*config.alpha);

    double rsum = 1.0;

    static vector<int> forward_from;
    forward_from.clear();
    forward_from.reserve(graph.n);
    forward_from.push_back(v);

    fwd_idx.first.clean();  //reserve
    fwd_idx.second.clean();  //residual
    fwd_idx.second.insert( v, rsum );

    

    if(config.with_rw_idx)
        rw_counter.reset_zero_values(); //store the pointers of each node's used rand-walk idxs 

    // for delta: try value from 1/4 to 1/n
    config.epsilon=config.epsilon/2;
    while( config.delta >= min_delta ){
        fora_topk_setting(graph.n, graph.m);
        num_iter_topk++;
        {
            Timer timer(FWD_LU);
            //INFO(config.rmax, graph.m*config.rmax, config.omega);
            if(graph.g[v].size()==0){
                rsum = 0.0;
                fwd_idx.first.insert(v, 1);
                compute_ppr_with_reserve();
                return;
            }else{
                forward_local_update_linear_topk( v, graph, rsum, config.rmax, lowest_delta_rmax, forward_from ); //forward propagation, obtain reserve and residual
            }
        }
        if(config.algo == LAZYUP){
			Timer timer(REBUILD_INDEX);	
			lazy_update_fwdidx(graph, theta);			
		}
        //i_destination_count.clean();
        //compute_ppr_with_fwdidx_new(graph, rsum);
        //compute_ppr_with_fwdidx_topk(graph, rsum);
        compute_ppr_with_fwdidx_topk(graph, rsum);

        
        {
            double kth_ppr_score = kth_ppr();

            //topk_ppr();

            //double kth_ppr_score = topk_pprs[config.k-1].second;
            if( kth_ppr_score >= (1+config.epsilon)*config.delta || config.delta <= min_delta ){  // once k-th ppr value in top-k list >= (1+epsilon)*delta, terminate
                INFO(kth_ppr_score, config.delta, rsum);
                break;
            }
            else{
                /*int j=0;
                for(; j<config.k; j++){
                    //INFO(topk_pprs[j].second, (1+config.epsilon)*config.delta);
                    if(topk_pprs[j].second<(1+config.epsilon)*config.delta)
                        break;
                }
                INFO("Our current accurate top-j", j);*/
                config.delta = max( min_delta, config.delta/4.0 );  // otherwise, reduce delta to delta/4
            }
        }
    }
	topk_ppr();
    config.epsilon=config.epsilon*2;
}

void get_topk(int v, Graph &graph){
	config.delta = 1.0/graph.n;
	fora_setting(graph.n, graph.m);
    //display_setting();
    if(config.algo == MC){
        montecarlo_query_topk(v, graph);
        topk_ppr();
    }
    else if(config.algo == BIPPR){
        bippr_query_topk(v, graph);
        topk_ppr();
    }
    else if(config.algo == FORA||config.algo == BATON||config.algo == FORA_NO_INDEX||config.algo == FORA_AND_BATON){
        if(config.opt&&config.algo == FORA)
            fora_query_topk_new(v, graph);
        else
            fora_query_topk_with_bound(v, graph);
        topk_ppr();
    }
    else if(config.algo == FWDPUSH){
        Timer timer(0);
        double rsum = 1;
        
        {
            Timer timer(FWD_LU);
            forward_local_update_linear(v, graph, rsum, config.rmax);
        }
        compute_ppr_with_reserve();
        topk_ppr();
    }
    else if(config.algo == HUBPPR){
        Timer timer(0);
        hubppr_query_topk_martingale(v, graph);
    }

     // not FORA, so it's single source
     // no need to change k to run again
     // check top-k results for different k
    if(config.algo != FORA  && config.algo != HUBPPR){
        compute_precision_for_dif_k(v);
    }
    compute_precision(v);

#ifdef CHECK_TOP_K_PPR
    //vector<pair<int, double>>& exact_result = exact_topk_pprs[v];
    INFO("query node:", v);
	cout<<"i: "<<topk_pprs.size()<<endl;
    for(int i=0; i<topk_pprs.size(); i++){
		//int hop=calc_hop(graph,v,topk_pprs[i].first);
		//Hop[hop]++;
        cout << "Estimated "<<i<<"-th node: " << topk_pprs[i].first << " PPR score: " << topk_pprs[i].second << " " //<< map_lower_bounds[topk_pprs[i].first].first<< " " << map_lower_bounds[topk_pprs[i].first].second
		<<endl;//<<" hop: "<<hop<<endl;    //<<" Exact k-th node: " << exact_result[i].first << " PPR score: " << exact_result[i].second << endl;
    }
	for(int i=0; i<11; i++){
		cout<<i<< " hop count :"<<Hop[i]<<endl; 
	}
#endif
}

void fwd_power_iteration(const Graph& graph, int start, unordered_map<int, double>& map_ppr){
    // static thread_local unordered_map<int, double> map_residual;
	unordered_map<int, double> map_residual;
    map_residual[start] = 1.0;

    int num_iter=0;
    double rsum = 1.0;
    while( num_iter < config.max_iter_num ){
        num_iter++;
        INFO(num_iter, rsum, map_residual.size());
        vector< pair<int,double> > pairs(map_residual.begin(), map_residual.end());
        map_residual.clear();
        for(const auto &p: pairs){
            if(p.second > 0){
                map_ppr[p.first] += config.alpha*p.second;
                int out_deg = graph.g[p.first].size();

                double remain_residual = (1-config.alpha)*p.second;
                rsum -= config.alpha*p.second;
                if(out_deg==0){
                    map_residual[start] += remain_residual;
                }
                else{
                    double avg_push_residual = remain_residual / out_deg;
                    for (int next : graph.g[p.first]) {
                        map_residual[next] += avg_push_residual;
                    }
                }
            }
        }
        pairs.clear();
    }
    map_residual.clear();
}

void multi_power_iter(const Graph& graph, const vector<int>& source, unordered_map<int, vector<pair<int ,double>>>& map_topk_ppr ){
    static thread_local unordered_map<int, double> map_ppr;
    for(int start: source){
        fwd_power_iteration(graph, start, map_ppr);

        vector<pair<int ,double>> temp_top_ppr(config.k);
        partial_sort_copy(map_ppr.begin(), map_ppr.end(), temp_top_ppr.begin(), temp_top_ppr.end(), 
            [](pair<int, double> const& l, pair<int, double> const& r){return l.second > r.second;});
        
        map_ppr.clear();
        map_topk_ppr[start] = temp_top_ppr;
    }
}

void gen_exact_topk(const Graph& graph){
    // config.epsilon = 0.5;
    // montecarlo_setting();

    vector<int> queries;
    load_ss_query(queries);
    INFO(queries.size());
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size );
    INFO(query_size);

    assert(config.k < graph.n-1);
    assert(config.k > 1);
    INFO(config.k);

    split_line();
    // montecarlo_setting();

    unsigned NUM_CORES = std::thread::hardware_concurrency()-1;
    assert(NUM_CORES >= 2);

    int num_thread = min(query_size, NUM_CORES);
    int avg_queries_per_thread = query_size/num_thread;

    vector<vector<int>> source_for_all_core(num_thread);
    vector<unordered_map<int, vector<pair<int ,double>>>> ppv_for_all_core(num_thread);

    for(int tid=0; tid<num_thread; tid++){
        int s = tid*avg_queries_per_thread;
        int t = s+avg_queries_per_thread;

        if(tid==num_thread-1)
            t+=query_size%num_thread;

        for(;s<t;s++){
            // cout << s+1 <<". source node:" << queries[s] << endl;
            source_for_all_core[tid].push_back(queries[s]);
        }
    }


    {
        Timer timer(PI_QUERY);
        INFO("power itrating...");
        std::vector< std::future<void> > futures(num_thread);
        for(int tid=0; tid<num_thread; tid++){
            futures[tid] = std::async( std::launch::async, multi_power_iter, std::ref(graph), std::ref(source_for_all_core[tid]), std::ref(ppv_for_all_core[tid]) );
        }
        std::for_each( futures.begin(), futures.end(), std::mem_fn(&std::future<void>::wait));
    }

    // cout << "average iter times:" << num_iter_topk/query_size << endl;
    cout << "average generation time (s): " << Timer::used(PI_QUERY)*1.0/query_size << endl;

    INFO("combine results...");
    for(int tid=0; tid<num_thread; tid++){
        for(auto &ppv: ppv_for_all_core[tid]){
            exact_topk_pprs.insert( ppv );
        }
        ppv_for_all_core[tid].clear();
    }

    save_exact_topk_ppr();
}

void topk(Graph& graph, vector<Query> list_query){
    vector<int> queries;
    // load_ss_query(queries);
    load_id_from_list(queries,list_query);
    INFO(queries.size());
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size );
    int used_counter=0;

    assert(config.k < graph.n-1);
    assert(config.k > 1);
    INFO(config.k);

    split_line();
    
    load_exact_topk_ppr();

     // not FORA, so it's single source
     // no need to change k to run again
     // check top-k results for different k
    if(config.algo != FORA && config.algo != HUBPPR){
        unsigned int step = config.k/5;
        if(step > 0){
            for(unsigned int i=1; i<5; i++){
                ks.push_back(i*step);
            }
        }
        ks.push_back(config.k);
        for(auto k: ks){	
            PredResult rst(0,0,0,0,0);
            pred_results.insert(MP(k, rst));
        }
    }

    used_counter = 0; 
    if(config.algo == FORA){
        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        rw_counter.init_keys(graph.n);
        upper_bounds.init_keys(graph.n);
        lower_bounds.init_keys(graph.n);
        ppr.initialize(graph.n);
		ppr_rw.initialize(graph.n);
        topk_filter.initialize(graph.n);
		//reuse_idx.initialize(graph.n);
		used_idx.reserve(graph.n);
		used_idx.assign(graph.n, 0);
		reuse_idx_vector.reserve(graph.n);
		reuse_idx_vector.assign(graph.n, 0);
		
		//reuse_idx.first.initialize(graph.n);
        //reuse_idx.second.initialize(graph.n);
    }
    else if(config.algo == MC){
        rw_counter.initialize(graph.n);
        ppr.initialize(graph.n);
        montecarlo_setting();
    }
    else if(config.algo == BIPPR){
        bippr_setting(graph.n, graph.m);
        rw_counter.initialize(graph.n);
        bwd_idx.first.initialize(graph.n);
        bwd_idx.second.initialize(graph.n);
        ppr.initialize(graph.n); 
    }
    else if(config.algo == FWDPUSH){
        fwdpush_setting(graph.n, graph.m);
        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        ppr.initialize(graph.n);
    }
    else if(config.algo == HUBPPR){
        hubppr_topk_setting(graph.n, graph.m);
        rw_counter.initialize(graph.n);
        upper_bounds.init_keys(graph.n);
        if(config.with_rw_idx){
            hub_counter.initialize(graph.n);
            load_hubppr_oracle(graph);
        }
        residual_maps.resize(graph.n);
        reserve_maps.resize(graph.n);
        map_lower_bounds.resize(graph.n);
        for(int v=0; v<graph.n; v++){
            residual_maps[v][v]=1.0;
            map_lower_bounds[v] = MP(v, 0);
        }
        updated_pprs.initialize(graph.n);
    }
    
    for(int i=0; i<query_size; i++){
        //cout << i+1 <<". source node:" << queries[i] << endl;
        std::chrono::steady_clock::time_point startTime;
        startTime = std::chrono::steady_clock::now();
        get_topk(queries[i], graph);


		/*
		for(int next : graph.g[queries[i]]){
			//cout<<next<<endl;
			get_topk(next, graph);
		}
		 */
        
        auto duration_query = get_duration(startTime);
        INFO(duration_query);
        split_line();
		 
    }

    cout << "average iter times:" << num_iter_topk/query_size << endl;
    display_time_usage(used_counter, query_size);
    set_result(graph, used_counter, query_size);

     //not FORA, so it's single source
     //no need to change k to run again
     // check top-k results for different k
    if(config.algo != FORA && config.algo != HUBPPR){
        display_precision_for_dif_k();
    }
}

void query(Graph& graph){
    INFO(config.algo);
    vector<int> queries;
    load_ss_query(queries);
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size );
    INFO(query_size);
    int used_counter=0;

    // assert(config.rw_cost_ratio >= 0);
    // INFO(config.rw_cost_ratio); 

    assert(config.rmax_scale >= 0);
    INFO(config.rmax_scale);

    // ppr.initialize(graph.n);
    ppr.init_keys(graph.n);
    // sfmt_init_gen_rand(&sfmtSeed , 95082);

    if(config.algo == BIPPR){ //bippr
        bippr_setting(graph.n, graph.m);
        display_setting();
        used_counter = BIPPR_QUERY;

        bwd_idx.first.initialize(graph.n);
        bwd_idx.second.initialize(graph.n);

        rw_counter.initialize(graph.n);
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            bippr_query(queries[i], graph);
            split_line();
        }
    }else if(config.algo == HUBPPR){
        bippr_setting(graph.n, graph.m);
        display_setting();
        used_counter = HUBPPR_QUERY;

        bwd_idx.first.initialize(graph.n);
        bwd_idx.second.initialize(graph.n);
        hub_counter.initialize(graph.n);
        rw_counter.initialize(graph.n);
        
        load_hubppr_oracle(graph);
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            hubppr_query(queries[i], graph);
            split_line();
        }
    }
    else if(config.algo == FORA){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();
        used_counter = FORA_QUERY;

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);

        // if(config.multithread)
        //     vec_ppr.resize(graph.n);

        // rw_counter.initialize(graph.n);
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            fora_query_basic(queries[i], graph);
            split_line();
        }
    }
	else if(config.algo == GENDA){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();
        used_counter = FORA_QUERY;

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		if(config.adaptive){
			config.update_size=0;
			set_optimal_beta(config,graph);
		}
		rebuild_idx(graph);
        // if(config.multithread)
        //     vec_ppr.resize(graph.n);

        // rw_counter.initialize(graph.n);
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            fora_query_basic(queries[i], graph);
            split_line();
        }
    }else if(config.algo == RESACC){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();
        used_counter = FORA_QUERY;

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);

        // if(config.multithread)
        //     vec_ppr.resize(graph.n);

        // rw_counter.initialize(graph.n);
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            resacc_query(queries[i], graph);
            split_line();
        }
    }else if(config.algo == MC){ //mc
        montecarlo_setting();
        display_setting();
        used_counter = MC_QUERY;

        rw_counter.initialize(graph.n);

        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            montecarlo_query(queries[i], graph);
            split_line();
        }
    }
    else if(config.algo == FWDPUSH){
        fwdpush_setting(graph.n, graph.m);
        display_setting();
        used_counter = FWD_LU;

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);

        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            Timer timer(used_counter);
            double rsum = 1;
            forward_local_update_linear(queries[i], graph, rsum, config.rmax);
            compute_ppr_with_reserve();
            split_line();
        }
    }

    display_time_usage(used_counter, query_size);
    set_result(graph, used_counter, query_size);
}

void batch_topk(Graph& graph){
    vector<int> queries;
    load_ss_query(queries);
    INFO(queries.size());
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size );
    int used_counter=0;

    assert(config.k < graph.n-1);
    assert(config.k > 1);
    INFO(config.k);

    split_line();

    used_counter = 0; 
    if(config.algo == FORA){
        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        rw_counter.init_keys(graph.n);
        upper_bounds.init_keys(graph.n);
        lower_bounds.init_keys(graph.n);
        ppr.initialize(graph.n);
        topk_filter.initialize(graph.n);
    }
    else if(config.algo == MC){
        rw_counter.initialize(graph.n);
        ppr.initialize(graph.n);
        montecarlo_setting();
    }
    else if(config.algo == BIPPR){
        bippr_setting(graph.n, graph.m);
        rw_counter.initialize(graph.n);
        bwd_idx.first.initialize(graph.n);
        bwd_idx.second.initialize(graph.n);  
        ppr.initialize(graph.n); 
    }
    else if(config.algo == FWDPUSH){
        fwdpush_setting(graph.n, graph.m);
        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        ppr.initialize(graph.n);
    }
    else if(config.algo == HUBPPR){
        hubppr_topk_setting(graph.n, graph.m);
        rw_counter.initialize(graph.n); 
        upper_bounds.init_keys(graph.n);
        if(config.with_rw_idx){
            hub_counter.initialize(graph.n);
            load_hubppr_oracle(graph);
        }
        residual_maps.resize(graph.n);
        reserve_maps.resize(graph.n);
        map_lower_bounds.resize(graph.n);
        for(int v=0; v<graph.n; v++){
            residual_maps[v][v]=1.0;
            map_lower_bounds[v] = MP(v, 0);
        }
        updated_pprs.initialize(graph.n);
    }

    unsigned int step = config.k/5;
    if(step > 0){
        for(unsigned int i=1; i<5; i++){
            ks.push_back(i*step);
        }
    }
    ks.push_back(config.k);
    for(auto k: ks){
        PredResult rst(0,0,0,0,0);
        pred_results.insert(MP(k, rst));
    }

    // not FORA, so it's of single source
    // no need to change k to run again
    // check top-k results for different k
    if(config.algo != FORA && config.algo != HUBPPR ){
        for(int i=0; i<query_size; i++){
            cout << i+1 <<". source node:" << queries[i] << endl;
            get_topk(queries[i], graph);
            split_line();
        }

        display_time_usage(used_counter, query_size);
        set_result(graph, used_counter, query_size);

        display_precision_for_dif_k();
    }
    else{ // for FORA, when k is changed, run algo again
        for(unsigned int k: ks){
            config.k = k;
            INFO("========================================");
            INFO("k is set to be ", config.k);
            result.topk_recall=0;
            result.topk_precision=0;
            result.real_topk_source_count=0;
            Timer::clearAll();
            for(int i=0; i<query_size; i++){
                cout << i+1 <<". source node:" << queries[i] << endl;
                get_topk(queries[i], graph);
                split_line();
            }
            pred_results[k].topk_precision=result.topk_precision;
            pred_results[k].topk_recall=result.topk_recall;
            pred_results[k].real_topk_source_count=result.real_topk_source_count;

            cout << "k=" << k << " precision=" << result.topk_precision/result.real_topk_source_count 
                              << " recall=" << result.topk_recall/result.real_topk_source_count << endl;
            cout << "Average query time (s):"<<Timer::used(used_counter)/query_size<<endl;
            Timer::reset(used_counter);
        }

        // display_time_usage(used_counter, query_size);
        display_precision_for_dif_k();
    }
}
float nextTime(double rateParameter) {
    
    return -log(1.0f - (double) rand() / (RAND_MAX + 1.0)) / rateParameter;
}

//-------------------------------------------------------------------------------
float nextTime_Geometric(double rateParameter){
    double prob=rateParameter;
    //printf("prob is %.12f\n", prob);
    double time=1.0;
    while(prob>=1){
      prob=prob/2.0;
      time=time/2.0;
    }
    double result=time;
    double temp_rand;
    while(true){
      temp_rand=(double)rand()/(RAND_MAX+1.0);
      if(temp_rand>prob){
        result+=time;
      }
      else{
        return result;
      }
    }
}

vector<double> Geometric(double lambda, double T){
    vector<double> sequence;
    double t=0;
    int num=0;
    while(t<T){
        double element=nextTime_Geometric(lambda);
        if(t+element<T){
            sequence.push_back(t+element);
        }
        t+=element;
        num++;
    }
    return sequence;
}
//-------------------------------------------------------------------------------
float nextTime_Even(double rateParameter){
    double time=1.0/rateParameter;
    return time;
}

vector<double> Even(double lambda, double T){
    vector<double> sequence;
    double t=0;
    int num=0;
    while(t<T){
        double element=nextTime_Even(lambda);
        if(t+element<T){
            sequence.push_back(t+element);
        }
        t+=element;
        num++;
    }
    return sequence;
}
//-------------------------------------------------------------------------------

vector<double> Normal(double expectation, double stan_devia, double T){
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<double> distribution(expectation, stan_devia);
    auto random_double = [&distribution, &gen]{ return distribution(gen); };

    vector<double> sequence;
    double t=0;
    int num=0;
    while(t<T){
        double element=random_double();
        if(element<0) element=0;
        if(element>2*expectation) element= 2*expectation;
        if(t+element<T){
            sequence.push_back(t+element);
        }
        t+=element;
        num++;
    }
    return sequence;
}
//-------------------------------------------------------------------------------

vector<double> Gamma(double alpha, double beta, double T){
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::gamma_distribution<> distribution(alpha, beta);
    vector<double> sequence;
    double t=0;
    int num=0;
    while(t<T){
        double element=distribution(gen);
        if(element<0) element=0;
        //if(element>2*expectation) element= 2*expectation;
        if(t+element<T){
            sequence.push_back(t+element);
        }
        t+=element;
        num++;
    }
    return sequence;
}
//-------------------------------------------------------------------------------

vector<double> poisson(double lambda, double T) {// T: seconds
    vector<double> sequence;
    double t=0;
//    srand(time(NULL));
    int num=0;
    while(t < T) {
        double element = nextTime(lambda);
        if(t+element < T) {
            sequence.push_back(t + element);
            //cout<< t+element<<endl;
        }
        t+=element;
        num++;
    }
    return sequence;
}
vector<Query> generate_query_workload_with_timestamp(double lambda, double T, int type, Graph& graph){
    vector<Query> list;
    INFO(lambda);
    INFO(T);
    seed +=1;
    srand(seed*4+type*10);
    vector<double> sequence;
    if(config.distribution=="poisson"){
        sequence = poisson(lambda, T);

    }else if(config.distribution=="geometric"){
        sequence=Geometric(lambda, T);
    }
    else if(config.distribution=="even"){
        sequence=Even(lambda, T);
    }
    else if(config.distribution=="normal"){
        sequence=Normal(1/lambda, 0.02, T);
    }
    else if(config.distribution=="gamma"){
        sequence=Gamma(1, 1/lambda, T);
    }
    
    INFO(sequence.size());
    
    int n = sequence.size();
    srand(1);
    for(int i=0; i < n; i++) {
        Query query(rand()%graph.n, sequence[i], type, 0);
        list.push_back(query);
        // INFO(query.node_id);    
    }
    return list;
}

vector<Query> load_query_workload_with_timestamp(int type){

    string query_file = config.graph_location + "ssquery.txt";
    vector<Query> list;
    FILE *fin = fopen(query_file.c_str(), "r");
    char t1[100], t2[100];
    int i = 0;
    while (fscanf(fin, "%s%s", t1, t2) != EOF) {
        assert(atoi(t1) < config.nodes);
        Query query(atoi(t1), atof(t2), type, 0);     
        list.push_back(query);
        i++;        
    }

    return list;
}
vector<Query> load_update_workload_with_timestamp(Graph graph, vector<pair<int,int>> &updates, int type){
    updates.clear();
    string query_file = config.graph_location + "update.txt";
    vector<Query> list;
    FILE *fin = fopen(query_file.c_str(), "r");
    char t1[100], t2[100],t3[100];
    int i = 0;
    bool if_delete;
    if_delete = ((rand()%1000)/1000.0 > config.insert_ratio);

    while (fscanf(fin, "%s%s%s", t1, t2, t3) != EOF) {
        assert(atoi(t1) < config.nodes);
        assert(atoi(t2) < config.nodes);
        Query query(atoi(t1), atof(t3), type, 0);     
        list.push_back(query);
        cout<<t1 <<" "<<t2<<" "<<t3<<endl;
        bool flag=false;
        int u = atoi(t1);
		int v = atoi(t2);
        

        pair<int,int> update;
        update.first=u;
        update.second=v;
        updates.push_back(update);
			
		
        i++;        
    }

    return list;
}
void show_list(vector<Query> list_query, vector<Query> list_update){
    for (int i =0; i<list_query.size();i++){
        cout<<"query "<< list_query[i].node_id<<"  "<< list_query[i].init_time<<endl;

    }
    for (int i =0; i<list_update.size();i++){
        cout<<"update "<< list_update[i].node_id<<"  "<< list_update[i].init_time<<endl;

    }
    for (int i =0; i<dynamic_workload.size();i++){
        cout<<"load "<< i<<"  "<< dynamic_workload[i]<<endl;

    }
}

void merge_dynamic_workload(vector<Query> &list_q, vector<Query> &list_u){
    dynamic_workload.clear();
    int query_size = list_q.size();
	int update_size = list_u.size();
	dynamic_workload.resize(query_size+update_size);
	dynamic_workload.assign(query_size+update_size,DQUERY);
    int i=0;//the posistion in query list
	int j=0;//the position in update list
	int k=0;//merged position
	while(i<query_size && j<update_size){
		if(list_q[i].init_time<=list_u[j].init_time){
			dynamic_workload[k++]=DQUERY;
			i++;
		}
		else{
			dynamic_workload[k++]=DUPDATE;
            // INFO(list_u[j].init_time);
			j++;
		}
	}
 
	while(i<query_size){
		dynamic_workload[k++]=DQUERY;
		i++;
	}
	while(j<update_size){
	    dynamic_workload[k++]=DUPDATE;
		j++;
	}  
}


void generate_dynamic_workload(bool if_hybrid=false){
	srand(10);
	int query_size = config.query_size;
	int update_size = config.update_size;
	dynamic_workload.resize(query_size+update_size);
	dynamic_workload.assign(query_size+update_size,DQUERY);
	dynamic_workload[0]=DUPDATE;
	for(int i=1; i<update_size; ){
		int n = rand()%(query_size+update_size);
		if(dynamic_workload[n]!=DUPDATE){
			i++;
			dynamic_workload[n]=DUPDATE;
		}
	}
	
	if(if_hybrid){
		int ss_size=query_size/3;
		int topk_size=(query_size-ss_size)/2;
		int onehop_size=query_size-ss_size-topk_size;
		INFO(ss_size,topk_size,onehop_size);
		for(int i=0; i<ss_size; ){
			int n = rand()%(query_size+update_size);
			if(dynamic_workload[n]==DQUERY){
				i++;
				dynamic_workload[n]=DSSQUERY;
			}
		}
		for(int i=0; i<topk_size; ){
			int n = rand()%(query_size+update_size);
			if(dynamic_workload[n]==DQUERY){
				i++;
				dynamic_workload[n]=DTOPKQUERY;
			}
		}
		for(int i=0; i<query_size+update_size; i++){
			if(dynamic_workload[i]==DQUERY){
				dynamic_workload[i]=DOHQUERY;
			}
		}
		for(int op : dynamic_workload){
			//cout << op << endl;
		}
	
	}
}
void assign_cost(vector<Query> &list_query, vector<Query> &list_update, vector<double> &query_costs,vector<double> &update_costs){
    for(int i = 0; i <list_query.size(); i++){
        list_query[i].process_time = query_costs[i];
        // cout<<"::::::"<<query_costs[i]<<endl;

    }
    for(int j = 0; j < list_update.size(); j++){
        list_update[j].process_time = update_costs[j];

    }

}
Query* getNext(vector<Query> &queryList, vector<Query> &insertList, int &i, int &j) {

    int kind=-1;
    int index=-1;
    double g_time=-1;

    if( i < queryList.size() ) {
        kind = 0;
        index= i;
        g_time = queryList[i].init_time;
    }

    if(j < insertList.size() ){
        if(kind == -1 || insertList[j].init_time < g_time)
        {
            kind = 1;
            index= j;
            g_time = insertList[j].init_time;
        }
    }

    if(kind == -1) return NULL;
    else
    {
        if(kind == 0) {
            i++;
            return &queryList[index];
        }else{
            j++;
            return &insertList[index];
        }
    }
}
pair<double, double> get_me_var(vector<double> query_costs){
    double sum = std::accumulate(std::begin(query_costs), std::end(query_costs), 0.0);

    double mean = sum / query_costs.size(); //mean

    double accum = 0.0;

    std::for_each (std::begin(query_costs), std::end(query_costs), [&](const double d) {

        accum += (d-mean)*(d-mean);

    });

    double var = accum/(query_costs.size()-1); //variance
    return pair<double,double>(mean, var);

}

double cal_error(int ds){
    double error = (ds - config.alpha*(1-config.alpha)*(ds-1))/ds-config.alpha;
    return error;

}
template <typename T>
void erase_indices(std::vector<T> &a, std::vector<int> &indices)
{
	for (size_t i = 0; i < indices.size(); ++i)
	{
		a.erase(a.begin() + indices[i] - i);
	}
	
}

void shuffle_sequence(Graph& graph, vector<Query> &list_query, vector<Query> &list_update, vector<pair<int,int>> updates){
    int query_size = list_query.size();
	int update_size = list_update.size();
    vector<Query> pending_update;
    int i=0;//the posistion in query list
	int j=0;//the position in update list
    vector<int> pop_update;
    while(i<query_size && j<update_size){
		if(list_query[i].init_time<=list_update[j].init_time){
            double e_sum = 0;
            int source_id = list_query[i].node_id;
            int out_neighbor_source = graph.g[source_id].size();
            // double error = (out_neighbor_source-config.alpha*(1-config.alpha)*(out_neighbor_source-1))/config.alpha/out_neighbor_source;
            for(int k=0; k<pending_update.size();k++){
                int update_id = pending_update[k].node_id;
                if(out_neighbor_source==0||graph.g[update_id].size()==0){
                    e_sum+=0;
                }else{
                    e_sum+= cal_error(out_neighbor_source)*(1-config.alpha*(1-config.alpha))/(graph.g[update_id].size()+1);
                }
                // cout<<"out_neighbor_source:"<<out_neighbor_source<<" graph.g[update_id].size():"<<graph.g[update_id].size()<<" e_sum: " <<e_sum<<endl;
            }
            cout<<i<<" out_neighbor_source:"<<out_neighbor_source<<" e_sum: " <<e_sum<<" list_query[i].init_time: "<<list_query[i].init_time<<endl;

            if(e_sum>config.e_sf&&config.e_sf>0){
                double low = list_query[i-1].init_time;
                double high = list_query[i].init_time;

                // cout<<" low:" <<low<<" high: "<<high<<endl;
                INFO(pending_update.size());
                INFO(j);
                
                for(int k=j-pending_update.size(); k<j-1;k++){
                    // list_update[k].init_time = low + rand() * (high - low) / RAND_MAX;     
                    // cout<<k<<" list_update[k].init_time:"<<list_update[k].init_time<<endl;
                    pop_update.push_back(k);
                }
                INFO(j-1);
                list_update[j-1].init_time = low + rand() * (high - low) / RAND_MAX;
                INFO(list_update[j-1].init_time);
                pending_update.clear();
                
            }
			i++;
		}
		else {
            // cout<<j<<" list_update[k].init_time:"<<list_update[j].init_time<<endl;
            list_update[j].node_id = updates[j].first;
            pending_update.push_back(list_update[j]);
			j++;
		}

	}
    if(config.e_sf>=1){
        list_update = generate_query_workload_with_timestamp(0, config.simulation_time, DUPDATE,graph);

    }else{
        erase_indices(list_update,pop_update);
    }
    
    for(int k=0; k<list_update.size();k++){
        INFO(list_update[k].init_time);
        
    }

    

}

void regenerate_updates(Graph graph,vector<pair<int,int>> &updates,vector<Query> &list_update){
	int n=graph.n;
    bool if_delete;
    srand(1);
    if_delete = ((rand()%1000)/1000.0 > config.insert_ratio);
	for(int i=0; i<list_update.size();){
		bool flag=false;
        int u = rand()%n;
		int v = rand()%n;
        
		if(u!=v){
			for (int next : graph.g[u]) {
				if(next==v){
					flag=true;
				}	
			}
			if(flag==if_delete){
				i++;
                if_delete = ((rand()%1000)/1000.0 > config.insert_ratio);
				pair<int,int> update;
                update.first=u;
                update.second=v;
                updates.push_back(update);
			}
		}
    }
}


pair<double, double> simulator_FIFO(vector<Query> &list_query, vector<Query> &list_update){
    assert(config.simulation_time < 10000);
    vector<double> simulated_query_costs;
    vector<double> simulated_update_costs;
    double total_time = config.simulation_time;
    int i=0, j=0;
    int count = 0; // the number of queries processed.

    double avg_response_time = 0.0;
    Query* current_query;

    double c_time=0;  // current time for processing queries
    
    while(c_time <= total_time) {
        current_query = getNext(list_query, list_update, i, j);
        if (current_query == NULL) {
            break;
        }
        // cout<<"c_time: "<<c_time<<"current_query->init_time: "<< current_query->init_time<<"current_query->process_time"<<current_query->process_time<<endl;
        c_time = max(c_time, current_query->init_time);
        
        if(c_time + current_query->process_time <= total_time) {
            if(current_query->operator_type == DQUERY) {
                count++;
                simulated_query_costs.push_back(current_query->process_time);
                avg_response_time +=  c_time -  current_query->init_time+ current_query->process_time;
            }
            else{
                simulated_update_costs.push_back(current_query->process_time);
            }
            c_time +=  current_query->process_time;
        } else {
            break;
        }
    }
    //cout<<"Avg_response_time " << avg_response_time / count / 1000000 <<endl;
    if(simulated_update_costs.size()<simulated_update_costs.size()*0.95)
        return pair<double,double>(count * 1.0 / config.simulation_time, -1);
    else
        return pair<double,double>(count * 1.0 / config.simulation_time, avg_response_time / count);
}
void dynamic_ssquery_origin(Graph& graph, vector<Query> list_query, vector<Query> list_update, vector<pair<int,int>> updates){
	query_costs.clear();
    update_costs.clear();
    qtau1_vector.clear();
    qtau2_vector.clear();
    qtau3_vector.clear();
    utau1_vector.clear();
    utau2_vector.clear();

    with_op = false;
    cout<<"Original start~~~"<<endl;
    vector<int> queries;
    // load_ss_query(queries);

    load_id_from_list(queries,list_query);
    INFO(queries.size());
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size);
    int used_counter=0;
	ppr.init_keys(graph.n);
	
	// load_update(updates);
    
    
	INFO(updates.size());
	
	bool IF_OUTPUT=false;
	int query_count=0;
	int update_count=0;
	int test_k=0;
    int true_value_count = 0;
	if(config.algo == FORA){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            if(i%1==0){
                cout<<"----------------------------------"<<endl;
			    cout<<"operation "<<i<<" update: "<<dynamic_workload[i]<<endl;
            }
            
			Timer timer(0);
            /*
			if(i!=(dynamic_workload.size()-1)){
				if(dynamic_workload[i+1]!=DQUERY&&dynamic_workload[i]!=DQUERY){
					continue;
				}
			}
            */
            std::chrono::steady_clock::time_point startTime;
            startTime = std::chrono::steady_clock::now();
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
                update_graph(graph, u, v);
				
				if(!config.exact&&config.with_rw_idx){
                    if(config.alter_idx == 0){
					    rebuild_idx(graph);
                    }
                    else{
                        rebuild_idx_vldb2010(graph, u, v, 1);
                    }
                    
				}
                auto duration_update = get_duration(startTime);
                update_costs.push_back(duration_update);
                INFO(duration_update);
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_basic(queries[query_count++], graph);
                auto duration_query = get_duration(startTime);
                
                query_costs.push_back(duration_query);
                INFO(duration_query);

                string lazyup_file =  config.graph_location +"/fora.txt";
                ofstream result_lazy_file(lazyup_file,ios::app);
                if(config.check_size>0){
                    if(true_value_count<config.check_size){
                        if(true_value_count==0){

                        result_lazy_file<<config.check_size<<endl;
                    }
                    output_imap(ppr, result_lazy_file, test_k);
                    INFO(true_value_count);
                    true_value_count++;
                    }

                }
			}
        }
		Timer::show();
		
		
        assign_cost(list_query, list_update, query_costs, update_costs);
        auto result = simulator_FIFO(list_query, list_update);
        final_throughput_ori.push_back(result.first);
        final_response_time_ori.push_back(result.second);
        config.mv_query = get_me_var(query_costs);
        config.mv_update = get_me_var(update_costs);
        qtau1 = get_me_var(qtau1_vector).first;
        qtau2 = get_me_var(qtau2_vector).first;



        string filename = config.graph_location + "result.txt";
        ofstream queryfile(filename, ios::app);
        queryfile<<"***average: "<<qtau1<<"  "<<qtau2<<"  "<<endl;
        queryfile<<"rmax: "<<config.rmax<<"alpha: "<<config.alpha<<" m: "<<graph.m<<" K: "<<config.omega<<" qtau1: "<<qtau1<<" qtau2: "<<qtau2<<"  "<<endl;
        queryfile<<" OA t_q: "<< config.mv_query.first<<" sqrt(var)/t_q"<< sqrt(config.mv_query.second)/config.mv_query.first <<"t_u: "<<
         config.mv_update.first<<" sqrt(var)/t_u"<< sqrt(config.mv_update.second)/config.mv_update.first<<" Throughput "<< 
         result.first<<" Average response time "<< result.second<<endl;
        // queryfile<< " Optimized beta = "<< opt_result.first<< "  "<< opt_result.second<<endl;
        
        queryfile.close();

	}
	else if(config.algo == BATON){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            cout<<"----------------------------------"<<endl;
			cout<<"operation "<<i<<endl;
			Timer timer(0);
            if(dynamic_workload[i]==DUPDATE){
				Timer timer(REBUILD_INDEX);
				int u,v;
                bool is_insert = true;
                u=updates[update_count].first;
                v=updates[update_count].second;
                update_count++;
                update_graph(graph, u, v);
                if(!config.exact&&config.with_rw_idx){
                    if(config.alter_idx == 0){
					    rebuild_idx(graph);
                    }
                    else{
                        rebuild_idx_vldb2010(graph, u, v, 1);
                    }
                    
				}
			}else if(dynamic_workload[i]==DQUERY){
				if(!config.exact){
					fora_query_basic(queries[query_count++], graph);
				}else{
					query_count++;
				}
			}
			
			
        }
		Timer::show();
		ofstream result_file;
		if(config.check_size>0){
			if(config.with_rw_idx){
				// Timer timer(REBUILD_INDEX);
				rebuild_idx(graph);
			}
			if(config.exact)
				result_file.open("result/"+config.graph_alias+"/exact.txt");
			else
				result_file.open("result/"+config.graph_alias+"/baton.txt");
			result_file<<config.check_size<<endl;
		}
        if(config.check_from!=0){
            query_count=config.check_from;
        }
		for(int i=0; i<config.check_size; i++){
			cerr<<i;
			if(config.power_iteration&&config.exact){
                // static thread_local unordered_map<int, double> map_ppr;
				unordered_map<int, double> map_ppr;
				{
					Timer timer(PI_QUERY);
					fwd_power_iteration(graph, queries[query_count++], map_ppr);
				}
				//Timer::show();
				vector<pair<int ,double>> temp_top_ppr;
				temp_top_ppr.clear();
				temp_top_ppr.resize(map_ppr.size());
				partial_sort_copy(map_ppr.begin(), map_ppr.end(), temp_top_ppr.begin(), temp_top_ppr.end(), 
					[](pair<int, double> const& l, pair<int, double> const& r){return l.second > r.second;});
				int non_zero_counter=0;
				for(long j=0; j<temp_top_ppr.size(); j++){
					if(temp_top_ppr[j].second>0)
						non_zero_counter++;
				}
				result_file << non_zero_counter << endl;
				for(int j=0; j< non_zero_counter; j++){
					result_file << j << "\t" << temp_top_ppr[j].first << "\t" << temp_top_ppr[j].second << endl;
				}
			}
			else {
				fora_query_basic(queries[query_count++], graph);
				output_imap(ppr, result_file, test_k);
			}
		}
		cout<<endl;
    }
	else if(config.algo == PARTUP){
	
        fora_setting(graph.n, graph.m);
        display_setting();
		
		reverse_idx.first.initialize(graph.n);
        reverse_idx.second.initialize(graph.n);
		fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
		int T_count=0;
		double T=16.0;
		double rsum_bound=graph.m*config.epsilon*config.epsilon/4/graph.n/(2*config.epsilon/3+2)/config.alpha/log(2/config.pfail);
		INFO(rsum_bound);
        for(int i=0; i<dynamic_workload.size(); i++){
			Timer timer(0);
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
				double errorlimit=config.epsilon*1.0/graph.n/config.alpha*graph.g[u].size()/T/rsum_bound;
				double epsrate=0.5;
				double sigma=0.5;
				
				if(T_count<T-4){
					
					if(errorlimit==0){
						continue;
					}
					
					INFO(errorlimit*epsrate);
					
					{
						Timer timer(11);
						reverse_push(u, graph, errorlimit*epsrate, 1);
					}
                    update_graph(graph, u, v);
					{
						Timer timer(12);
						if(reverse_idx.first.occur.m_num>1+graph.gr[u].size()){
							T_count++;
							for(long j=0; j<reverse_idx.first.occur.m_num; j++){
								long id = reverse_idx.first.occur[j];
								//cout<<"id: "<<id<<"  reserve: "<<reverse_idx.first[id]<<" residue "<<reverse_idx.second[id]<<endl;
								if(reverse_idx.first[id]>errorlimit*(1-epsrate)){
									update_idx(graph, id);
								}
							}
						}else{
							for(long j=0; j<reverse_idx.first.occur.m_num; j++){
								long id = reverse_idx.first.occur[j];
									update_idx(graph, id);
							}
						}
					}
				}else{
					Timer timer(REBUILD_INDEX);
					T_count=0;
					update_graph(graph, u, v);
					rebuild_idx(graph);
				}
				
				
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_dynamic(queries[query_count++], graph, T_count/T);
			}
        }
		if(config.check_size>0){
			Timer::show();
			ofstream result_file;
			result_file.open("result/"+config.graph_alias+"/partup.txt");
			result_file<<config.check_size<<endl;
			for(int i=0; i<config.check_size; i++){
				cerr<<i;
				fora_query_dynamic(queries[query_count++], graph,  T_count/T);
				output_imap(ppr, result_file, test_k);
			}
			cout<<endl;
		}
    }
    else if(config.algo == LAZYUP){//**************************************Used*********************
        fora_setting(graph.n, graph.m);
        
		fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        reverse_idx.first.initialize(graph.n);
        reverse_idx.second.initialize(graph.n);
		inacc_idx.reserve(graph.n);
		inacc_idx.assign(graph.n, 1);
		double theta=0.8;
        config.theta = theta;
        Timer timer(0);

        // double errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n;
        // double epsrate=0.5;
        //----some bugs maybe, here is the parameters setting
        double errorlimit=1.0/graph.n;
        double epsrate=1;
        config.rbmax = config.test_beta1*errorlimit*epsrate;
        int true_value_count = 0;
        

        display_setting();
        for(int i=0; i<dynamic_workload.size(); i++){
            if(i%config.show_each==0){
                cout<<"----------------------------------"<<endl;
                cout<<"operation "<<i<<" update: "<<dynamic_workload[i]<<endl;
            }
			std::chrono::steady_clock::time_point startTime;
            startTime = std::chrono::steady_clock::now();
            
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
                
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;

                
                double errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n;
                double epsrate=1;
                config.test_beta1 = 1.0/2.0;
                config.rbmax = config.test_beta1*errorlimit*epsrate;


				// if(graph.n < 300000){
				// 	errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n*10;
                //     // INFO(errorlimit);
				// }
                
                

				if(errorlimit==0){
					continue;
				}
				
                
				{
					reverse_push(u, graph, config.rbmax, 1);
                    
				}
                auto duration_update1 = get_duration(startTime);
                // INFO(duration_update1);
                utau1_vector.push_back(duration_update1);
                // utau1 = max(duration_update1, utau1);
                

				update_graph(graph, u, v);
				
				{
					//-------some bugs maybe
                    // INFO(reverse_idx.first.occur.m_num);
                    // INFO(graph.n);
					for(long j=0; j<graph.n; j++){
                    //long id = reverse_idx.first.occur[j];
                        long id = j;
                        double pmin;
                        if(reverse_idx.first.exist(id))
                            pmin=min((reverse_idx.first[id]+config.rbmax)*(1-config.alpha)/config.alpha,1.0);
                        else
                            pmin=min((config.rbmax)*(1-config.alpha)/config.alpha,1.0);
                        //double pmin=(reverse_idx.first[id]+errorlimit*epsrate)/config.alpha;
                        inacc_idx[id]*=(1-pmin/graph.g[u].size());
                        // cout<<"inacc_idx[id]::"<<inacc_idx[id]<<endl;
                    }

                    // for(long j=0; j<reverse_idx.first.occur.m_num; j++){
                    // long id = reverse_idx.first.occur[j];
                    // double pmin=min((reverse_idx.first[id]+errorlimit*epsrate)*(1-config.alpha)/config.alpha,1.0);
                    // inacc_idx[id]*=(1-pmin/graph.g[u].size());
                    // }

                    
				}
                auto duration_update2 = get_duration(startTime);
                // INFO(duration_update2);
                utau2_vector.push_back(duration_update2-duration_update1);
                // utau2 = max(duration_update2-duration_update1, utau2);
                Timer timer(11);
                update_costs.push_back(duration_update2);
                
                
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_lazy_dynamic(queries[query_count++], graph, theta);
                Timer timer(12);
                //show query time
                auto duration_query = get_duration(startTime);
                
                query_costs.push_back(duration_query);
                if(config.show_each<10){
                    INFO(duration_query);
                }

                // // *******Obtain the true value of PPR***************************
                // string exact_file =  config.graph_location +"/exact.txt";
                // ofstream result_file(exact_file,ios::app);
                // if(config.check_size>0){

                //     if(true_value_count<config.check_size){
                //         if(true_value_count==0){
                //             result_file<<config.check_size<<endl;
                //         }
                        
                //         INFO(true_value_count);
                //         true_value_count++;
                        
                //         // static thread_local unordered_map<int, double> map_ppr;
                //         unordered_map<int, double> map_ppr;
                //         {
                //             Timer timer(PI_QUERY);
                //             fwd_power_iteration(graph, queries[query_count-1], map_ppr);
                //         }
                //         //Timer::show();
                //         vector<pair<int ,double>> temp_top_ppr;
                //         temp_top_ppr.clear();
                //         temp_top_ppr.resize(map_ppr.size());
                //         partial_sort_copy(map_ppr.begin(), map_ppr.end(), temp_top_ppr.begin(), temp_top_ppr.end(), 
                //             [](pair<int, double> const& l, pair<int, double> const& r){return l.second > r.second;});
                //         int non_zero_counter=0;
                //         for(long j=0; j<temp_top_ppr.size(); j++){
                //             if(temp_top_ppr[j].second>0)
                //                 non_zero_counter++;
                //         }
                //         INFO(non_zero_counter);
                //         result_file << non_zero_counter << endl;
                //         for(int j=0; j< non_zero_counter; j++){
                //             result_file << j << "\t" << temp_top_ppr[j].first << "\t" << temp_top_ppr[j].second << endl;
                //         }
                        
                //     }
                    
                
                // }
                // result_file.close();
                // // ****** end of true value*******
                
			}
        }
        
        ASSERT(query_costs.size()==list_query.size()&&update_costs.size()==list_update.size());
        assign_cost(list_query, list_update, query_costs, update_costs);
        auto result = simulator_FIFO(list_query, list_update);
        config.mv_query = get_me_var(query_costs);
        config.mv_update = get_me_var(update_costs);
        qtau1 = get_me_var(qtau1_vector).first;
        qtau2 = get_me_var(qtau2_vector).first;
        qtau3 = get_me_var(qtau3_vector).first;
        utau1 = get_me_var(utau1_vector).first;
        utau2 = get_me_var(utau2_vector).first;
        maxqtau1 = *max_element(qtau1_vector.begin(), qtau1_vector.end());
        maxqtau2 = *max_element(qtau2_vector.begin(), qtau2_vector.end());
        maxqtau3 = *max_element(qtau3_vector.begin(), qtau3_vector.end());
        maxutau1 = *max_element(utau1_vector.begin(), utau1_vector.end());
        maxutau2 = *max_element(utau2_vector.begin(), utau2_vector.end());
        a_query1.push_back(qtau1); w_query1.push_back(maxqtau1);
        a_query2.push_back(qtau2); w_query2.push_back(maxqtau2);
        a_query3.push_back(qtau3); w_query3.push_back(maxqtau3);
        a_update1.push_back(utau1); w_update1.push_back(maxutau1);
        a_update2.push_back(utau2); w_update2.push_back(maxutau2);
        cout<<"********************************************************************"<<endl;
        
        
        cout<<"Query size: "<< list_query.size()<<endl;
        cout<<"Query mean: "<< config.mv_query.first<<"variance: "<<config.mv_query.second<<endl;

        cout<<"Update size: "<< list_update.size()<<endl;
        cout<<"Update mean: "<< config.mv_update.first<<"variance: "<<config.mv_update.second<<endl;

        cout<<"Throughput "<< result.first<< endl;
        cout<<"Response time "<< result.second<< endl;
        
        double t_sum = config.mv_query.first*config.lambda_q+config.mv_update.first*config.lambda_u;
        if(t_sum>=1){
            INFO(t_sum);
            crowd_flag = 1;
            cerr<<"Arrival rates for query and update are too high!  Lambda_q = "<< config.lambda_q <<endl;
            // exit(0);
        }
        
        final_throughput_ori.push_back(result.first);
        final_response_time_ori.push_back(result.second);
        
        auto opt_result = improve_throughput();
        
        final_beta1_ori.push_back(opt_result.first);
        final_beta2_ori.push_back(opt_result.second);
        

        // output the log file
        string filename = config.graph_location + "result.txt";
        ofstream queryfile(filename, ios::app);
        queryfile<<"***average: "<<qtau1<<"  "<<qtau2<<"  "<<qtau3<<"  "<<utau1<<"  "<<utau2<<"  "<<endl;
        queryfile<<"maximum: "<<maxqtau1<<"  "<<maxqtau2<<"  "<<maxqtau3<<"  "<<maxutau1<<"  "<<maxutau2<<"  "<<endl;
        queryfile<<" OA t_q: "<< config.mv_query.first<<"t_u: "<<
         config.mv_update.first<<" Throughput "<< 
         result.first<<" Average response time "<< result.second<< "  Crowded = "<< t_sum <<endl;
        queryfile<< " Optimized beta = "<< opt_result.first<< "  "<< opt_result.second<<endl;
        
        queryfile.close();
        cout<<"Response time "<< result.second<< endl;
        config.beta1 = opt_result.first;
        config.beta2 = opt_result.second;
        
		// if(config.check_size>0){
		// 	// Timer::show();
		// 	ofstream result_file;
		// 	result_file.open("result/"+config.graph_alias+"/lazyup.txt");
		// 	result_file<<config.check_size<<endl;

        //     if(config.check_from!=0){
        //         INFO(config.check_from);
        //         query_count=config.check_from;
        //     }
		// 	for(int i=0; i<config.check_size; i++){
		// 		cerr<<i;
		// 		fora_query_lazy_dynamic(queries[query_count++], graph, theta);
		// 		output_imap(ppr, result_file, test_k);
		// 	}
		// 	cout<<endl;
		// }
    }
	else if(config.algo == RESACC){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            cout<<"----------------------------------"<<endl;
			cout<<"operation "<<i<<endl;
			Timer timer(0);
            std::chrono::steady_clock::time_point startTime;
            startTime = std::chrono::steady_clock::now();
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
				update_graph(graph, u, v);
                auto duration_update = get_duration(startTime);
                update_costs.push_back(duration_update);
                INFO(duration_update);
			}else if(dynamic_workload[i]==DQUERY){
				resacc_query(queries[query_count++], graph);
                auto duration_query = get_duration(startTime);
                
                query_costs.push_back(duration_query);
                INFO(duration_query);
			}
        }
		Timer::show();
		ofstream result_file;
		result_file.open("result/"+config.graph_alias+"/resacc.txt");
		result_file<<config.check_size<<endl;
        if(config.check_from!=0){
                INFO(config.check_from);
                query_count=config.check_from;
            }
		for(int i=0; i<config.check_size; i++){
			cerr<<i;
			resacc_query(queries[query_count++], graph);
			output_imap(ppr, result_file, test_k);
		}
		
        assign_cost(list_query, list_update, query_costs, update_costs);
        auto result = simulator_FIFO(list_query, list_update);
        final_throughput_ori.push_back(result.first);
        final_response_time_ori.push_back(result.second);
	}
	// set_result(graph, used_counter, query_size);
    // if(config.algo == LAZYUP){
    //     ASSERT(query_costs.size()==list_query.size()&&update_costs.size()==list_update.size());
    //     assign_cost(list_query, list_update, query_costs, update_costs);
    //     auto result = simulator_FIFO(list_query, list_update);
    //     config.mv_query = get_me_var(query_costs);
    //     config.mv_update = get_me_var(update_costs);

    //     cout<<"********************************************************************"<<endl;
    //     // cout<<qtau1<<"  "<<qtau2<<"  "<<qtau3<<"  "<<utau1<<"  "<<utau2<<"  "<<endl;
    //     cout<<"Query size: "<< list_query.size()<<endl;
    //     cout<<"Query mean: "<< config.mv_query.first<<"variance: "<<config.mv_query.second<<endl;

    //     cout<<"Update size: "<< list_update.size()<<endl;
    //     cout<<"Update mean: "<< config.mv_update.first<<"variance: "<<config.mv_update.second<<endl;

    //     cout<<"Throughput "<< result.first<< endl;
    //     cout<<"Response time "<< result.second<< endl;
        
    //     double t_sum = config.mv_query.first*config.lambda_q+config.mv_update.first*config.lambda_u;
    //     if(t_sum>=1){
    //         INFO(t_sum);
    //         crowd_flag = 1;
    //         cerr<<"Arrival rates for query and update are too high!  Lambda_q = "<< config.lambda_q <<endl;
    //         // exit(0);
    //     }
        
    //     final_throughput_ori.push_back(result.first);
    //     final_response_time_ori.push_back(result.second);
    //     // output the log file
    //     string filename = config.graph_location + "result.txt";
    //     ofstream queryfile(filename, ios::app);
        
    //     queryfile<<"***" <<" OA t_q: "<< config.mv_query.first<<"t_u: "<<
    //      config.mv_update.first<<" Throughput "<< 
    //      result.first<<" Average response time "<< result.second<< "  Crowded = "<< t_sum <<endl;
        
    //     queryfile.close();

    // }
	cout<<"trw:"<<num_total_rw<<endl;
	cout<<"hrw:"<<num_hit_idx<<endl;
}

void dynamic_ssquery_with_op(Graph& graph, vector<Query> list_query, vector<Query> list_update,vector<pair<int,int>> updates){
	// reset the vectors
    query_costs.clear();
    update_costs.clear();
    qtau1_vector.clear();
    qtau2_vector.clear();
    qtau3_vector.clear();
    utau1_vector.clear();
    utau2_vector.clear();

    cout<<"After optimization~~~"<<endl;
    with_op = true;

    

    vector<int> queries;
    // load_ss_query(queries);

    load_id_from_list(queries,list_query);
    INFO(queries.size());
    unsigned int query_size = queries.size();
    query_size = min( query_size, config.query_size );
    int used_counter=0;
	ppr.init_keys(graph.n);
	
	// load_update(updates);
    
    
	INFO(updates.size());
	
	bool IF_OUTPUT=false;
	int query_count=0;
	int update_count=0;
	int test_k=0;
	if(config.algo == FORA){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            cout<<"----------------------------------"<<endl;
			cout<<"operation "<<i<<endl;
			Timer timer(0);
            /*
			if(i!=(dynamic_workload.size()-1)){
				if(dynamic_workload[i+1]!=DQUERY&&dynamic_workload[i]!=DQUERY){
					continue;
				}
			}
            */
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
                update_graph(graph, u, v);
				
				if(!config.exact&&config.with_rw_idx){
                    if(config.alter_idx == 0){
					    rebuild_idx(graph);
                    }
                    else{
                        rebuild_idx_vldb2010(graph, u, v, 1);
                    }
                    
				}
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_basic(queries[query_count++], graph);
			}
        }
		Timer::show();
		ofstream result_file;
		result_file.open("result/"+config.graph_alias+"/fora.txt");
		result_file<<config.check_size<<endl;
        if(config.check_from!=0){
            INFO(config.check_from);
            query_count=config.check_from;
        }
		for(int i=0; i<config.check_size; i++){
			cerr<<i;
			fora_query_basic(queries[query_count++], graph);
			output_imap(ppr, result_file, test_k);
		}
		cout<<endl;
	}
	else if(config.algo == BATON){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            cout<<"----------------------------------"<<endl;
			cout<<"operation "<<i<<endl;
			Timer timer(0);
            if(dynamic_workload[i]==DUPDATE){
				Timer timer(REBUILD_INDEX);
				int u,v;
                bool is_insert = true;
                u=updates[update_count].first;
                v=updates[update_count].second;
                update_count++;
                update_graph(graph, u, v);
                if(!config.exact&&config.with_rw_idx){
                    if(config.alter_idx == 0){
					    rebuild_idx(graph);
                    }
                    else{
                        rebuild_idx_vldb2010(graph, u, v, 1);
                    }
                    
				}
			}else if(dynamic_workload[i]==DQUERY){
				if(!config.exact){
					fora_query_basic(queries[query_count++], graph);
				}else{
					query_count++;
				}
			}
			
			
        }
		Timer::show();
		ofstream result_file;
		if(config.check_size>0){
			if(config.with_rw_idx){
				Timer timer(REBUILD_INDEX);
				rebuild_idx(graph);
			}
			if(config.exact)
				result_file.open("result/"+config.graph_alias+"/exact.txt");
			else
				result_file.open("result/"+config.graph_alias+"/baton.txt");
			result_file<<config.check_size<<endl;
		}
        if(config.check_from!=0){
            query_count=config.check_from;
        }
		for(int i=0; i<config.check_size; i++){
			cerr<<i;
			if(config.power_iteration&&config.exact){
                static thread_local unordered_map<int, double> map_ppr;
				// unordered_map<int, double> map_ppr;
				{
					Timer timer(PI_QUERY);
					fwd_power_iteration(graph, queries[query_count++], map_ppr);
				}
				//Timer::show();
				vector<pair<int ,double>> temp_top_ppr;
				temp_top_ppr.clear();
				temp_top_ppr.resize(map_ppr.size());
				partial_sort_copy(map_ppr.begin(), map_ppr.end(), temp_top_ppr.begin(), temp_top_ppr.end(), 
					[](pair<int, double> const& l, pair<int, double> const& r){return l.second > r.second;});
				int non_zero_counter=0;
				for(long j=0; j<temp_top_ppr.size(); j++){
					if(temp_top_ppr[j].second>0)
						non_zero_counter++;
				}
				result_file << non_zero_counter << endl;
				for(int j=0; j< non_zero_counter; j++){
					result_file << j << "\t" << temp_top_ppr[j].first << "\t" << temp_top_ppr[j].second << endl;
				}
			}
			else {
				fora_query_basic(queries[query_count++], graph);
				output_imap(ppr, result_file, test_k);
			}
		}
		cout<<endl;
    }
	else if(config.algo == PARTUP){
	
        fora_setting(graph.n, graph.m);
        display_setting();
		
		reverse_idx.first.initialize(graph.n);
        reverse_idx.second.initialize(graph.n);
		fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
		int T_count=0;
		double T=16.0;
		double rsum_bound=graph.m*config.epsilon*config.epsilon/4/graph.n/(2*config.epsilon/3+2)/config.alpha/log(2/config.pfail);
		INFO(rsum_bound);
        for(int i=0; i<dynamic_workload.size(); i++){
			Timer timer(0);
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
				double errorlimit=config.epsilon*1.0/graph.n/config.alpha*graph.g[u].size()/T/rsum_bound;
				double epsrate=0.5;
				double sigma=0.5;
				
				if(T_count<T-4){
					
					if(errorlimit==0){
						continue;
					}
					
					INFO(errorlimit*epsrate);
					
					{
						Timer timer(11);
						reverse_push(u, graph, errorlimit*epsrate, 1);
					}
                    update_graph(graph, u, v);
					{
						Timer timer(12);
						if(reverse_idx.first.occur.m_num>1+graph.gr[u].size()){
							T_count++;
							for(long j=0; j<reverse_idx.first.occur.m_num; j++){
								long id = reverse_idx.first.occur[j];
								//cout<<"id: "<<id<<"  reserve: "<<reverse_idx.first[id]<<" residue "<<reverse_idx.second[id]<<endl;
								if(reverse_idx.first[id]>errorlimit*(1-epsrate)){
									update_idx(graph, id);
								}
							}
						}else{
							for(long j=0; j<reverse_idx.first.occur.m_num; j++){
								long id = reverse_idx.first.occur[j];
									update_idx(graph, id);
							}
						}
					}
				}else{
					Timer timer(REBUILD_INDEX);
					T_count=0;
					update_graph(graph, u, v);
					rebuild_idx(graph);
				}
				
				
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_dynamic(queries[query_count++], graph, T_count/T);
			}
        }
		if(config.check_size>0){
			Timer::show();
			ofstream result_file;
			result_file.open("result/"+config.graph_alias+"/partup.txt");
			result_file<<config.check_size<<endl;
			for(int i=0; i<config.check_size; i++){
				cerr<<i;
				fora_query_dynamic(queries[query_count++], graph,  T_count/T);
				output_imap(ppr, result_file, test_k);
			}
			cout<<endl;
		}
    }
    else if(config.algo == LAZYUP){//**************************************Used*********************
        fora_setting(graph.n, graph.m);
        display_setting();
		fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
        reverse_idx.first.initialize(graph.n);
        reverse_idx.second.initialize(graph.n);
		inacc_idx.reserve(graph.n);
		inacc_idx.assign(graph.n, 1);
		double theta=0.8;
        config.theta=theta;
        // double errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n;
        // double epsrate=0.5;
        //----some bugs maybe
        double errorlimit=1.0/graph.n;
        double epsrate=1;
        int true_value_count = 0;
        config.rbmax = config.beta2*errorlimit*epsrate;
        // config.rbmax = 1.0/20.0*errorlimit*epsrate;
        config.beta = config.beta1*config.beta;
        
        
        Timer timer(0);
        display_setting();
        for(int i=0; i<dynamic_workload.size(); i++){
            if(i%config.show_each==0){
                cout<<"----------------------------------"<<endl;
                cout<<"operation "<<i<<endl;
            }
			std::chrono::steady_clock::time_point startTime;
            startTime = std::chrono::steady_clock::now();
            
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
                // INFO(config.rbmax);
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
				
				// if(graph.n < 300000){
				// 	errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n*10;
                //     // INFO(errorlimit);
				// }

                
                // double errorlimit=double(std::max(1,(int)(graph.g[u].size())))/graph.n;
                // double epsrate=1;
                // config.test_beta1 = 1.0/2.0;
                // config.rbmax = config.test_beta1*config.rbmax;
                
				if(errorlimit==0){
					continue;
				}

                
				{
					reverse_push(u, graph, config.rbmax, 1);
                    
				}
                auto duration_update1 = get_duration(startTime);
                // INFO(duration_update1);
                utau1_vector.push_back(duration_update1);
                // utau1 = max(duration_update1, utau1);
				update_graph(graph, u, v);
				
				{
					
					//-------some bugs maybe
					for(long j=0; j<graph.n; j++){
                    //long id = reverse_idx.first.occur[j];
                        long id = j;
                        double pmin;
                        if(reverse_idx.first.exist(id))
                            pmin=min((reverse_idx.first[id]+config.rbmax)*(1-config.alpha)/config.alpha,1.0);
                        else
                            pmin=min((config.rbmax)*(1-config.alpha)/config.alpha,1.0);
                        //double pmin=(reverse_idx.first[id]+errorlimit*epsrate)/config.alpha;
                        inacc_idx[id]*=(1-pmin/graph.g[u].size());
                    }

                    // for(long j=0; j<reverse_idx.first.occur.m_num; j++){
                    // long id = reverse_idx.first.occur[j];
                    // double pmin=min((reverse_idx.first[id]+errorlimit*epsrate)*(1-config.alpha)/config.alpha,1.0);
                    // inacc_idx[id]*=(1-pmin/graph.g[u].size());
                    // }
                    
				}
                auto duration_update2 = get_duration(startTime);
                // INFO(duration_update2);
                utau2_vector.push_back(duration_update2-duration_update1);
                // utau2 = max(duration_update2-duration_update1, utau2);
                Timer timer(11);
                update_costs.push_back(duration_update2);
                
                
			}else if(dynamic_workload[i]==DQUERY){
				fora_query_lazy_dynamic(queries[query_count++], graph, theta);
                Timer timer(12);
                //show query time
                auto duration_query = get_duration(startTime);
                
                query_costs.push_back(duration_query);
                if(config.show_each<10){
                    INFO(duration_query);
                }

                string lazyup_file =  config.graph_location +"/lazyup.txt";
                // ofstream result_lazy_file(lazyup_file,ios::trunc);

                ofstream result_lazy_file;
		        result_lazy_file.open(lazyup_file, ios::app);

                if(config.check_size>0){
                    if(true_value_count<config.check_size){
                        if(true_value_count==0){

                        result_lazy_file<<config.check_size<<endl;
                    }
                    output_imap(ppr, result_lazy_file, test_k);
                    INFO(true_value_count);
                    true_value_count++;
                    }

                }
                // result_lazy_file.close();
                
			}
        }
        ASSERT(query_costs.size()==list_query.size()&&update_costs.size()==list_update.size());
        assign_cost(list_query, list_update, query_costs, update_costs);
        auto result = simulator_FIFO(list_query, list_update);
        config.mv_query = get_me_var(query_costs);
        config.mv_update = get_me_var(update_costs);
        qtau1 = get_me_var(qtau1_vector).first;
        qtau2 = get_me_var(qtau2_vector).first;
        qtau3 = get_me_var(qtau3_vector).first;
        utau1 = get_me_var(utau1_vector).first;
        utau2 = get_me_var(utau2_vector).first;

        cout<<"********************************************************************"<<endl;
        cout<<qtau1<<"  "<<qtau2<<"  "<<qtau3<<"  "<<utau1<<"  "<<utau2<<"  "<<endl;
        cout<<"Query size: "<< list_query.size()<<endl;
        cout<<"Query mean: "<< config.mv_query.first<<"variance: "<<config.mv_query.second<<endl;

        cout<<"Update size: "<< list_update.size()<<endl;
        cout<<"Update mean: "<< config.mv_update.first<<"variance: "<<config.mv_update.second<<endl;

        cout<<"Throughput "<< result.first<< endl;
        cout<<"Response time "<< result.second<< endl;
        // cout<<" updates[43].first "<< updates[43].first<<endl;
        double t_sum = config.mv_query.first*config.lambda_q+config.mv_update.first*config.lambda_u;
        if(t_sum>=1){
            INFO(t_sum);
            crowd_flag = 1;
            cerr<<"Arrival rates for query and update are too high!  Lambda_q = "<< config.lambda_q <<endl;
            // exit(0);
        }
        
        final_throughput.push_back(result.first);
        final_response_time.push_back(result.second);
        
        // auto opt_result = improve_throughput();
        // final_beta1.push_back(opt_result.first);
        // final_beta2.push_back(opt_result.second);
        

        // output the log file
        string filename = config.graph_location + "result.txt";
        ofstream queryfile(filename, ios::app);
        queryfile<<"******tau: "<<qtau1<<"  "<<qtau2<<"  "<<qtau3<<"  "<<utau1<<"  "<<utau2<<"  "<<endl;
        queryfile<<"IA t_q: "<< config.mv_query.first<<"t_u: "<<
         config.mv_update.first<<" Throughput "<< 
         result.first<<" Average response time "<< result.second<< "  Crowded = "<< t_sum <<"\n"<<
         "\n"<<"\n"<<endl;



        // queryfile<< " Optimized beta = "<< opt_result.first<< "  "<< opt_result.second<<endl;
        
        queryfile.close();
        // if(config.test_throughput==true){
        //     config.beta1 = opt_result.first;
        //     config.beta2 = opt_result.second;

        // }
        
		// if(config.check_size>0){
		// 	// Timer::show();
		// 	ofstream result_file;
		// 	result_file.open("result/"+config.graph_alias+"/lazyup.txt");
		// 	result_file<<config.check_size<<endl;

        //     if(config.check_from!=0){
        //         INFO(config.check_from);
        //         query_count=config.check_from;
        //     }
		// 	for(int i=0; i<config.check_size; i++){
		// 		cerr<<i;
		// 		fora_query_lazy_dynamic(queries[query_count++], graph, theta);
		// 		output_imap(ppr, result_file, test_k);
		// 	}
		// 	cout<<endl;
		// }
    }
	else if(config.algo == RESACC){ //fora
        fora_setting(graph.n, graph.m);
        display_setting();

        fwd_idx.first.initialize(graph.n);
        fwd_idx.second.initialize(graph.n);
		
        for(int i=0; i<dynamic_workload.size(); i++){
            cout<<"----------------------------------"<<endl;
			cout<<"operation "<<i<<endl;
			Timer timer(0);
            if(dynamic_workload[i]==DUPDATE){
				int u,v;
				u=updates[update_count].first;
				v=updates[update_count].second;
				update_count++;
				update_graph(graph, u, v);
			}else if(dynamic_workload[i]==DQUERY){
				resacc_query(queries[query_count++], graph);
			}
        }
		Timer::show();
		ofstream result_file;
		result_file.open("result/"+config.graph_alias+"/resacc.txt");
		result_file<<config.check_size<<endl;
        if(config.check_from!=0){
                INFO(config.check_from);
                query_count=config.check_from;
            }
		for(int i=0; i<config.check_size; i++){
			cerr<<i;
			resacc_query(queries[query_count++], graph);
			output_imap(ppr, result_file, test_k);
		}
		cout<<endl;
	}
	// set_result(graph, used_counter, query_size);
	cout<<"trw:"<<num_total_rw<<endl;
	cout<<"hrw:"<<num_hit_idx<<endl;
}
#endif //FORA_QUERY_H
