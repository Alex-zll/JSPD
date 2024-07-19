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
		int Jsize; //�����С
		int Dsize; //����Я�����ݴ�С
		int gen; //ȷ���������
		std::unordered_set<int> onM; //����Щ������ִ��
	};
	using tasks = std::vector<task>; //tasks[i]��Ҫ�� jid��Ӧ

	struct machine {
		MachineId mid;
		int power; //��ʾ��������������
	};
	using machines = std::vector<machine>; //ͬ���±���mid��Ӧ

	struct disk {
		DiskId did;
		int IOspeed;
		int capcity;
	};
	using disks = std::vector<disk>; //�±���did��Ӧ


	//������Ҫ��->�Ӻ���->������˸��ڵ�������ڽӱ����
	
	

	void solveJSDP(tasks tks, machines mchns, disks dsks, std::vector<std::unordered_set<JobId>> env_dep,
		std::vector<std::unordered_set<JobId>> dat_dep);
}


#endif // !JOB_SCHEDULING_AND_DATA_ASSIGNMENT_PROBLEM_H

