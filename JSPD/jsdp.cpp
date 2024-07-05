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
			vector<int> start_time(l, -1); //start_time[i]��ʾ����i�Ŀ�ʼʱ��
			vector<DiskId> assignD(l, -1); //assignD[i]��ʾ����i�����disk
			vector<int> readSpeed(l, 0); //readSpeed[i]��ʾ����i��һ�׶ε�ʱ��
			vector<MachineId> assignM(l, -1); //assignM[i]��ʾ����i����Ĵ������
			vector<pair<JobId, int>> exeing(n); //exeing[i]��ʾ����i���һ�����е�����Ϳ�ʼ���е�ʱ��(�ڶ��׶εĿ�ʼʱ��)
			vector<int> remCaps(m, 0); //ȷ��disk��ʣ������

			for (int midx = 0; midx < n; ++midx) {
				exeing[midx].first = -1;
			}

			for (int didx = 0; didx < m; ++didx) {
				remCaps[didx] = dsks[didx].capcity;
			}

			//�����ʼ��
			// ���ﲻ�ԣ�Ӧ��ͬʱ�������ݺͻ���������ȷ��˳�򣬶��������ν���
			//1�����ȸ�����������ȷ������˳��
			unordered_map<int, vector<JobId>> data_map; //����������ȷ���ĳ���˳�� 
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
			
			vector<unordered_set<JobId>> par_env_dep(l), par_dat_dep(l); //��->��
			for (JobId jid = 0; jid < l; ++jid) {
				for (auto chd = dat_dep[jid].begin(); chd != dat_dep[jid].end(); ++chd) {
					par_dat_dep[*chd].insert(jid);
				}
				for (auto chd = env_dep[jid].begin(); chd != env_dep[jid].end(); ++chd)
					par_env_dep[*chd].insert(jid);
			}

			// 2��3���Է���һ����д���ͬʱ��Ҫ֪�� ��->�������
			//2����ζ���˳����ͬ���������û���������һ��ȷ��˳��
			//3�����ΰ���˳����ȷ�����ܸ��ŵ�disk��machine
			for (int idx = 0; idx < data_map.size(); idx++) {
				// 1��ȷ����ǰ�����£��Ƿ��и���һ�������й�ϵ  
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

				// 2����ȷ��ÿ�������ڵ�ǰ����µ����翪ʼʱ��
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
							
							// ���׵Ľ���ʱ�����ظ����㣿����������������
							int par_strt = start_time[*parj];
							int rdSpd = readSpeed[*parj];
							if (par_strt != -1) {
								int fsh = par_strt + rdSpd; //fsh������ʼʱ��+��ȡʱ��+ִ��ʱ��+д��ʱ��
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

						// 3�����ȷ��disk��machine�ķ����������������Ŀ�ʼʱ�������С�ĸĶ�
						// disk�ķ������ȿ����ٶ�
						int Dsize = tks[*j1].Dsize;
						DiskId maxDsk = -1;
						int maxspd = 0;
						
						// ����������һ���ж�����
						for (DiskId didx = 0; didx < dsks.size(); ++didx) {
							if (remCaps[didx] > Dsize && dsks[didx].IOspeed > maxspd) {
								maxspd = dsks[didx].IOspeed;
								maxDsk = didx;
							}
						}
						remCaps[maxDsk] -= Dsize;
						assignD[*j1] = maxDsk;

						// �������ʱ����Ҫȷ��������ǰ�Ƿ���к���Ҫ�ȴ���õ�ʱ����ܵõ���Խ�����Ļ���
						// ���ȷ����ǰ�����Ƿ���У�
						int mstrt = strt + readSpeed[*j1];
						unordered_set<MachineId> Ms = tks[*j1].onM;
						int maxpwr = 0, maxpwr2 = 0;
						MachineId bestM1 = -1, bestM2 = -1;
						int new_strt = 9999999;
						for (auto mch = Ms.begin(); mch != Ms.end(); ++mch) {
							if (exeing[*mch].first == -1 && mchns[*mch].power > maxpwr) { //û�����������������ִ��
								maxpwr = mchns[*mch].power;
								bestM1 = *mch;
							}
							else if (exeing[*mch].first != -1) {
								JobId ej = exeing[*mch].first;
								int strtT = exeing[*mch].second, exeT = 0;
								if (tks[ej].Jsize % mchns[*mch].power != 0) exeT += 1;
								exeT += tks[ej].Jsize / mchns[*mch].power;
								if (strtT + exeT <= mstrt) { //�������
									if (mchns[*mch].power > maxpwr) {
										maxpwr = mchns[*mch].power;
										bestM1 = *mch;
									}
								}
								else { //��ǰ������
									if (strtT + exeT < new_strt) { //����������Ļ���
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

			//4���õ���ʼ��
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