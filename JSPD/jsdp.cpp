#include "jsdp.h"

#include <random>
#include <unordered_map>
#include <queue>
#include <utility>
#include <iostream>
#include <algorithm>

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

			// 给Dsize更高的任务优先分配高性能Disk
			for (int j = 0; j < l; ++j) {
				tks[j].gen = dat_dep[j].size() + env_dep[j].size() + 1;
			}
			tasks tmp_tks = tks;
			sort(tmp_tks.begin(), tmp_tks.end(), [](task tk1, task tk2){
				return tk1.Dsize * tk1.gen > tk2.Dsize * tk2.gen;
			});
			for (int idx = 0; idx < l; ++idx) {
				JobId j1 = tmp_tks[idx].jid;
				int Dsize = tmp_tks[idx].Dsize;
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
				assignD[j1] = maxDsk;
			}

			//构造初始解
			unordered_map<int, vector<JobId>> data_map; //由数据依赖和环境依赖确定的顺序 
			vector<int> data_inD(l, 0), vis(l, 0), env_inD(l, 0);
			for (int j = 0; j < dat_dep.size(); ++j) {
				for (auto inj = dat_dep[j].begin(); inj != dat_dep[j].end(); ++inj) {
					data_inD[*inj] += 1;
				}
				for (auto inj = env_dep[j].begin(); inj != env_dep[j].end(); ++inj)
					env_inD[*inj] += 1;
			}

			queue<JobId> myQ;
			for (int j = 0; j < l; ++j) {
				if (data_inD[j] == 0 && env_inD[j] == 0)
					myQ.push(j);
			}
			int seq = 0;
			while (!myQ.empty()) {
				vector<JobId> seqj;
				int size = myQ.size();
				seq += 1;
				unordered_set <JobId> inDj;

				while (size--) {
					JobId curJ = myQ.front();
					myQ.pop();
					seqj.push_back(curJ);
					for (auto nxtJ = dat_dep[curJ].begin(); nxtJ != dat_dep[curJ].end(); ++nxtJ) {
						data_inD[*nxtJ]--;
						inDj.insert(*nxtJ);
					}

					for (auto nxtJ = env_dep[curJ].begin(); nxtJ != env_dep[curJ].end(); ++nxtJ) {
						env_inD[*nxtJ]--;
						inDj.insert(*nxtJ);
					}
				}

				for (auto J = inDj.begin(); J != inDj.end(); ++J)
					if (data_inD[*J] == 0 && env_inD[*J] == 0) myQ.push(*J);
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
			//3、依次按照顺序来确定性能更优的disk和machine
			for (int idx = 0; idx < data_map.size(); idx++) {
				vector<JobId> jList = data_map[idx + 1];
				tasks tmp_tsks;
				for (auto j1 = jList.begin(); j1 != jList.end(); ++j1) {
					task tmp_tk = tks[*j1];
					tmp_tsks.push_back(tmp_tk);
				}
				sort(tmp_tsks.begin(), tmp_tsks.end(), [](task tk1, task tk2) {
					return tk1.Jsize * tk1.gen > tk2.Jsize * tk2.gen;
					});
				// 1、确定每个任务在当前情况下的最早开始时间
				for (int idx = 0; idx < tmp_tsks.size(); ++idx) {
					JobId j1 = tmp_tsks[idx].jid;
					auto parL1 = par_env_dep[j1], parL2 = par_dat_dep[j1];
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
							readSpeed[j1] += wrt;
						}
					}
					start_time[j1] = strt;

					// 3、最后确定machine的分配情况，其中任务的开始时间可能做小的改动
					int mstrt = strt;
					unordered_set<MachineId> Ms = tks[j1].onM;
					//int maxpwr = 0, maxpwr2 = 0;
					int fast_fnsh1 = 99999999, fast_fnsh2 = 999999999;
					MachineId bestM1 = -1, bestM2 = -1;
					int new_strt = 9999999;
					for (auto mch = Ms.begin(); mch != Ms.end(); ++mch) {
						//if (exeing[*mch].first == -1 && mchns[*mch].power > maxpwr) { //没有任务在这个机器上执行
						//	maxpwr = mchns[*mch].power;
						//	bestM1 = *mch;
						//}
						int cur_fnshT = strt + readSpeed[j1];
						int cur_exeT = 0, cur_wrtT = 0;
						if (tks[j1].Jsize % mchns[*mch].power != 0) cur_exeT += 1;
						cur_exeT += tks[j1].Jsize / mchns[*mch].power;

						if (tks[j1].Dsize % dsks[assignD[j1]].IOspeed != 0) cur_wrtT += 1;
						cur_wrtT = tks[j1].Dsize / dsks[assignD[j1]].IOspeed;
						cur_fnshT += (cur_exeT + cur_wrtT);
						if (exeing[*mch].first == -1) { //没有任务在这个机器上执行
							if (cur_fnshT < fast_fnsh1) {
								bestM1 = *mch;
								fast_fnsh1 = cur_fnshT;
							}
							
						}
						else if (exeing[*mch].first != -1) {
							JobId ej = exeing[*mch].first;
							int strtT = exeing[*mch].second, exeT = 0;
							int rdSpd = readSpeed[ej], wrt = 0;
							if (tks[ej].Dsize % dsks[assignD[ej]].IOspeed != 0) wrt += 1;
							wrt += tks[ej].Dsize / dsks[assignD[ej]].IOspeed;

							if (tks[ej].Jsize % mchns[*mch].power != 0) exeT += 1;
							exeT += tks[ej].Jsize / mchns[*mch].power;
							int pfnsh = strtT + rdSpd + exeT + wrt;
							if (pfnsh <= strt) { //这里空闲
								/*if (mchns[*mch].power > maxpwr) {
									maxpwr = mchns[*mch].power;
									bestM1 = *mch;
								}*/
								if (cur_fnshT < fast_fnsh1) {
									bestM1 = *mch;
									fast_fnsh1 = cur_fnshT;
								}
							}
							else { //当前不空闲
								//if (strtT + rdSpd + exeT + wrt < new_strt) { //找最早结束的机器
								//	bestM2 = *mch;
								//	new_strt = strtT + rdSpd + exeT + wrt;
								//}
								new_strt = strtT + rdSpd + exeT + wrt;
								cur_fnshT -= strt;
								cur_fnshT += new_strt;
								if (cur_fnshT < fast_fnsh2) {
									bestM2 = *mch;
									mstrt = new_strt;
									fast_fnsh2 = cur_fnshT;
								}
							}
						}
					}
					/*if (bestM1 != -1) {
						assignM[j1] = bestM1;
						exeing[bestM1].first = j1;
						exeing[bestM1].second = mstrt;
					}
					else {
						assignM[j1] = bestM2;
						exeing[bestM2].first = j1;
						exeing[bestM2].second = new_strt;
						strt = new_strt;
						start_time[j1] = strt;
					}*/
					if (fast_fnsh2 < fast_fnsh1) {
						assignM[j1] = bestM2;
						exeing[bestM2].first = j1;
						exeing[bestM2].second = mstrt;
						start_time[j1] = mstrt;
					}
					else {
						assignM[j1] = bestM1;
						exeing[bestM1].first = j1;
						exeing[bestM1].second = strt;
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