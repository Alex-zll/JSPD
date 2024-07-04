#include "jsdp.h"

#include <random>
#include <unordered_map>
#include <queue>

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
			vector<int> start_time(l); //start_time[i]��ʾ����i�Ŀ�ʼʱ��
			vector<DiskId> assignD(l); //assignD[i]��ʾ����i�����disk
			vector<MachineId> assignM(l); //assignM[i]��ʾ����i����Ĵ������

			//�����ʼ��
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
			  

			//4���õ���ʼ��
		}
	};

	void solveJSDP(tasks tks, machines mchns, disks dsks, vector<std::unordered_set<JobId>> env_dep,
		vector<std::unordered_set<JobId>> dat_dep) {
		Solver().solveProblem(tks, mchns, dsks, env_dep, dat_dep);
	}
}