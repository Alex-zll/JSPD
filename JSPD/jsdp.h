#pragma once

#ifndef JOB_SCHEDULING_AND_DATA_ASSIGNMENT_PROBLEM_H
#define JOB_SCHEDULING_AND_DATA_ASSIGNMENT_PROBLEM_H

#include<vector>
#include<unordered_map>
#include<unordered_set>

namespace jsdp {
	using JobId = int;
	using MachineId = int;
	using DiskId = int;

	struct task {
		JobId jid;
		int Jsize; //任务大小
		int Dsize; //任务携带数据大小
		int gen; //确定后代数量
		std::unordered_set<int> onM; //在哪些机器上执行
	};
	using tasks = std::vector<task>; //tasks[i]需要与 jid对应

	struct machine {
		MachineId mid;
		int power; //表示机器的运行能力
	};
	using machines = std::vector<machine>; //同样下标与mid对应

	struct disk {
		DiskId did;
		int IOspeed;
		int capcity;
	};
	using disks = std::vector<disk>; //下标与did对应


	//依赖需要父->子和子->父，因此父节点数组和邻接表都设计
	
	

	void solveJSDP(tasks tks, machines mchns, disks dsks, std::vector<std::unordered_set<JobId>> env_dep,
		std::vector<std::unordered_set<JobId>> dat_dep);
}


#endif // !JOB_SCHEDULING_AND_DATA_ASSIGNMENT_PROBLEM_H

