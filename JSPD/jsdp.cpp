#include "jsdp.h"

#include <random>
#include <unordered_map>
#include <queue>
#include <utility>
#include <iostream>

using namespace std;

namespace jsdp {
	class Solver {
		// random number generator.
		mt19937 pseudoRandNumGen;
		void initRand(int seed) { pseudoRandNumGen = mt19937(seed); }
		int fastRand(int lb, int ub) { return (pseudoRandNumGen() % (ub - lb)) + lb; }
		int fastRand(int ub) { return pseudoRandNumGen() % ub; }
		int rand(int lb, int ub) { return uniform_int_distribution<int>(lb, ub - 1)(pseudoRandNumGen); }
		int rand(int ub) { return uniform_int_distribution<int>(0, ub - 1)(pseudoRandNumGen); }
	public:
		void solveProblem(tasks tks, machines mchns, disks dsks, vector<std::unordered_set<JobId>> env_dep,
			vector<std::unordered_set<JobId>> dat_dep) {
			int l = tks.size(), n = mchns.size(), m = dsks.size();
			vector<int> start_time(l, -1); //start_time[i]表示任务i的开始时间
			vector<DiskId> assignD(l, -1); //assignD[i]表示任务i分配的disk
			vector<int> readSpeed(l, 0); //readSpeed[i]表示任务i第一阶段的时间
			vector<MachineId> assignM(l, -1); //assignM[i]表示任务i分配的处理机器
			vector<pair<JobId, int>> exeing(n); //exeing[i]表示机器i最近一次运行的任务和开始运行的时间(第二阶段的开始时间)
			vector<int> remCaps(m, 0); //确定disk的剩余容量

			for (int midx = 0; midx < n; ++midx) {
				exeing[midx].first = -1;
			}

			for (int didx = 0; didx < m; ++didx) {
				remCaps[didx] = dsks[didx].capcity;
			}

			//构造初始解
			// 这里不对，应该同时根据数据和环境依赖来确定顺序，而不能依次进行
			//1、首先根据数据依赖确定初步顺序
			unordered_map<int, vector<JobId>> data_map; //由数据依赖确定的初步顺序 
			vector<int> inD(l, 0), vis(l, 0);
			for (int j = 0; j < dat_dep.size(); ++j) {
				for (auto inj = dat_dep[j].begin(); inj != dat_dep[j].end(); ++inj) {
					inD[*inj] += 1;
				}
			}

			queue<JobId> myQ;
			for (int j = 0; j < dat_dep.size(); ++j) {
				if (inD[j] == 0) 
					myQ.push(j);
			}
			int seq = 0;
			while (!myQ.empty()) {
				vector<JobId> seqj;
				int size = myQ.size();
				seq += 1;

				while (size--) {
					JobId curJ = myQ.front();
					myQ.pop();
					seqj.push_back(curJ);
					for (auto nxtJ = dat_dep[curJ].begin(); nxtJ != dat_dep[curJ].end(); ++nxtJ) {
						if (--inD[*nxtJ] == 0) myQ.push(*nxtJ);
					}
				}
				data_map[seq] = seqj;
			}
			
			vector<unordered_set<JobId>> par_env_dep(l), par_dat_dep(l); //子->父
			for (JobId jid = 0; jid < l; ++jid) {
				for (auto chd = dat_dep[jid].begin(); chd != dat_dep[jid].end(); ++chd) {
					par_dat_dep[*chd].insert(jid);
				}
				for (auto chd = env_dep[jid].begin(); chd != env_dep[jid].end(); ++chd)
					par_env_dep[*chd].insert(jid);
			}

			// 2和3可以放在一起进行处理，同时需要知道 子->父的情况
			//2、其次对于顺序相同的任务，利用环境依赖进一步确定顺序
			//3、依次按照顺序来确定性能更优的disk和machine
			for (int idx = 0; idx < data_map.size(); idx++) {
				// 1、确定当前次序下，是否还有更进一步的序列关系  
				vector<JobId> jList = data_map[idx + 1];
				unordered_map<JobId, int> env_map1;
				unordered_map<int, vector<JobId>> env_map2;
				for (int id1 = 0; id1 < jList.size(); ++id1)
					env_map1[jList[id1]] = 0;
				for (int id1 = 0; id1 < jList.size(); ++id1) {
					for (int id2 = id1 + 1; id2 < jList.size(); ++id2) {
						JobId j1 = jList[id1], j2 = jList[id2];
						if (env_dep[j1].find(j2) != env_dep[j1].end()) env_map1[j2] = env_map1[j1] + 1;
						else if (env_dep[j2].find(j1) != env_dep[j2].end()) env_map1[j1] = env_map1[j2] + 1;
					}
				}
				for (auto jmp = env_map1.begin(); jmp != env_map1.end(); ++jmp) {
					env_map2[(*jmp).second].push_back((*jmp).first);
				}

				// 2、再确定每个任务在当前情况下的最早开始时间
				for (int idx2 = 0; idx2 < env_map2.size(); ++idx2) {
					vector<JobId> jbl = env_map2[idx2];
					for (auto j1 = jbl.begin(); j1 != jbl.end(); ++j1) {
						auto parL1 = par_env_dep[*j1], parL2 = par_dat_dep[*j1];
						int strt = 0;

						for (auto parj = parL1.begin(); parj != parL1.end(); ++parj) {
							int par_strt = start_time[*parj];
							int rdSpd = readSpeed[*parj];
							if (par_strt != -1) {
								int fsh = par_strt + rdSpd;
								int exet = 0;
								if (tks[*parj].Jsize % mchns[assignM[*parj]].power != 0) exet += 1;
								exet += tks[*parj].Jsize / mchns[assignM[*parj]].power;
								fsh += exet;
								if (fsh > strt) strt = fsh;
							}
						}

						for (auto parj = parL2.begin(); parj != parL2.end(); ++parj) {
							
							// 父亲的结束时间在重复计算？？？？？？？？？
							int par_strt = start_time[*parj];
							int rdSpd = readSpeed[*parj];
							if (par_strt != -1) {
								int fsh = par_strt + rdSpd; //fsh包含开始时间+读取时间+执行时间+写入时间
								int exet = 0;
								int wrt = 0;
								if (tks[*parj].Jsize % mchns[assignM[*parj]].power != 0) exet += 1;
								if (tks[*parj].Dsize % dsks[assignD[*parj]].IOspeed != 0) wrt += 1;
								exet += tks[*parj].Jsize / mchns[assignM[*parj]].power;
								wrt += tks[*parj].Dsize / dsks[assignD[*parj]].IOspeed;
								fsh += exet;
								fsh += wrt;
								if (fsh > strt) strt = fsh;

								readSpeed[*j1] += wrt;
							}
						}

						start_time[*j1] = strt;

						// 3、最后确定disk和machine的分配情况，其中任务的开始时间可能做小的改动
						// disk的分配优先考虑速度
						int Dsize = tks[*j1].Dsize;
						DiskId maxDsk = -1;
						int maxspd = 0;
						
						// 这里可以添加一个判断条件
						for (DiskId didx = 0; didx < dsks.size(); ++didx) {
							if (remCaps[didx] > Dsize && dsks[didx].IOspeed > maxspd) {
								maxspd = dsks[didx].IOspeed;
								maxDsk = didx;
							}
						}
						remCaps[maxDsk] -= Dsize;
						assignD[*j1] = maxDsk;

						// 分配机器时，需要确定机器当前是否空闲和需要等待多久的时间才能得到相对较理想的机器
						// 如何确定当前机器是否空闲？
						int mstrt = strt + readSpeed[*j1];
						unordered_set<MachineId> Ms = tks[*j1].onM;
						int maxpwr = 0, maxpwr2 = 0;
						MachineId bestM1 = -1, bestM2 = -1;
						int new_strt = 9999999;
						for (auto mch = Ms.begin(); mch != Ms.end(); ++mch) {
							if (exeing[*mch].first == -1 && mchns[*mch].power > maxpwr) { //没有任务在这个机器上执行
								maxpwr = mchns[*mch].power;
								bestM1 = *mch;
							}
							else if (exeing[*mch].first != -1) {
								JobId ej = exeing[*mch].first;
								int strtT = exeing[*mch].second, exeT = 0;
								if (tks[ej].Jsize % mchns[*mch].power != 0) exeT += 1;
								exeT += tks[ej].Jsize / mchns[*mch].power;
								if (strtT + exeT <= mstrt) { //这里空闲
									if (mchns[*mch].power > maxpwr) {
										maxpwr = mchns[*mch].power;
										bestM1 = *mch;
									}
								}
								else { //当前不空闲
									if (strtT + exeT < new_strt) { //找最早结束的机器
										bestM2 = *mch;
										new_strt = strtT + exeT;
									}
								}
							}
						}
						if (bestM1 != -1) {
							assignM[*j1] = bestM1;
							exeing[bestM1].first = *j1;
							exeing[bestM1].second = mstrt;
						}
						else {
							assignM[*j1] = bestM2;
							exeing[bestM2].first = *j1;
							exeing[bestM2].second = new_strt;
							strt = new_strt - readSpeed[*j1];
							start_time[*j1] = strt;
						}
					}
				}
				
			}

			//4、得到初始解
			for (int j = 0; j < l; ++j) {
				cout << j + 1 << ' ' << start_time[j] << ' ' << assignM[j] + 1 << ' ' << assignD[j] + 1 << endl;
			}
		}
	};

	void solveJSDP(tasks tks, machines mchns, disks dsks, vector<std::unordered_set<JobId>> env_dep,
		vector<std::unordered_set<JobId>> dat_dep) {
		Solver().solveProblem(tks, mchns, dsks, env_dep, dat_dep);
	}
}