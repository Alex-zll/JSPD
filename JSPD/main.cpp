#include <iostream>
#include <string>
#include <sstream>

#include "jsdp.h"

using namespace std;
using namespace jsdp;

void loadInput(tasks &tks, machines &mchns, disks &dsks, vector<std::unordered_set<JobId>> &env_dep,
    vector<std::unordered_set<JobId>> &dat_dep) {
   
    int l, m, n;
    //处理task
    cin >> l;
    cin.ignore();
    for (int i = 1; i <= l; ++i) {
        task tk;
        string line;
        getline(cin, line);
        istringstream iss(line);
        vector<int> nums;
        int num;
        while (iss >> num) {
            nums.push_back(num);
        }
        tk.jid = nums[0] - 1;
        tk.Jsize = nums[1];
        tk.Dsize = nums[2];
        for (int j = 4; j < nums.size(); ++j) {
            tk.onM.insert(nums[j] - 1);
        }
        tks.push_back(tk);
    }

    //处理machine
    cin >> n;
    for (int i = 1; i <= n; ++i) {
        machine mchn;
        cin >> mchn.mid >> mchn.power;
        mchn.mid -= 1;
        mchns.push_back(mchn);
    }

    //处理disk
    cin >> m;
    for (int i = 1; i <= m; ++i) {
        disk dsk;
        cin >> dsk.did >> dsk.IOspeed >> dsk.capcity;
        dsk.did -= 1;
        dsks.push_back(dsk);
    }

    env_dep.resize(l);
    dat_dep.resize(l);
    int N, M;
    //处理data-dependencies
    cin >> N;
    for (int itr = 0; itr < N; ++itr) {
        int i, j;
        cin >> i >> j;
        dat_dep[i - 1].insert(j - 1);
    }

    //处理environment-dependencies
    cin >> M;
    for (int itr = 0; itr < M; ++itr) {
        int i, j;
        cin >> i >> j;
        env_dep[i - 1].insert(j - 1);
    }

}

int main() {
    tasks tks;
    machines mchns;
    disks dsks;
    vector<std::unordered_set<JobId>> env_dep, dat_dep; //父->子

    loadInput(tks, mchns, dsks, env_dep, dat_dep);
    solveJSDP(tks, mchns, dsks, env_dep, dat_dep);

    return 0;
}
