#include "bits/stdc++.h"
#include <cuda_runtime.h>
#include <sys/time.h>
#define in std::cin
#define out std::cout
#define rep(i,N) for(LL i=0;i<N;++i)
typedef long long int LL;

#define CHECK(call)																			\
{																							\
	const cudaError_t error = call;															\
	if (error != cudaSuccess)																\
	{																						\
		out << "Error: " << __FILE__ << ":" << __LINE__ << ", ";							\
		out << "code:" << error << ", reason: " << cudaGetErrorString(error) << std::endl;	\
		exit(1);																			\
	}																						\
}																							\

double cpuSecond()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}

const LL inf = RAND_MAX;

void solveOnCPU(LL N, LL A[])
{
	std::sort(A, A + N);
}

__global__ void solveOnGPU(LL A[], const LL block, const LL step)
{
	LL idx = blockIdx.x * blockDim.x + threadIdx.x;
	LL e = (idx ^ step);
	if (e > idx)
	{
		LL v1 = A[idx];
		LL v2 = A[e];
		if (((idx & block) != 0 && v1 < v2)
			|| ((idx & block) == 0) && v1 > v2)
		{
			A[e] = v1;
			A[idx] = v2;
		}
	}
}

int main()
{
	LL S, T, P;
	in >> S >> T >> P;

	std::ofstream writing_file;
	writing_file.open("output.txt", std::ios::trunc);

	// ブロック・スレッド数の上限を取得
	int dev = 0;
	cudaSetDevice(dev);
	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, dev);
	LL maxThreads = deviceProp.maxThreadsPerBlock;
	out << "GPU Device : " << deviceProp.name << ", maxThreads : " << maxThreads << std::endl;

	out << "N\tCPUTime\t\tGPUTime\t\tStatus\t\tRatio\t\tWinner" << std::endl;
	for (LL N = S; N <= T; N += P)
	{
		// メモリを確保
		LL newN = 1;
		while (newN < N) newN *= 2;
		size_t nBytes = sizeof(LL) * N, newNBytes = sizeof(LL) * newN;
		LL *h_A, *h_B, *d_B, *cpuRes, *gpuRes;
		h_A = (LL *)malloc(nBytes);
		h_B = (LL *)malloc(nBytes);
		cpuRes = (LL *)malloc(nBytes);
		gpuRes = (LL *)malloc(nBytes);
		CHECK(cudaMalloc((LL**)&d_B, newNBytes));

		// 数列を初期化
		srand((unsigned int)time(0));
		rep(i, N) h_A[i] = rand();
		
		// CPU 処理時間を計測
		auto cpuStartTime = cpuSecond();
		
			memcpy(h_B, h_A, nBytes);
			solveOnCPU(N, h_B);
			memcpy(cpuRes, h_B, nBytes);

		auto cpuEndTime = cpuSecond();

		// GPU 処理時間を計測
		auto gpuStartTime = cpuSecond();
		
			LL *temp;
			temp = (LL *)malloc(newNBytes);
			memcpy(temp, h_A, nBytes);
			for (LL i = N; i < newN; ++i) temp[i] = inf;
			CHECK(cudaMemcpy(d_B, temp, newNBytes, cudaMemcpyHostToDevice));
			for (LL block = 2; block <= newN; block *= 2)
			{
				for (LL step = block / 2; step >= 1; step /= 2)
				{
					solveOnGPU<<< std::max(newN / maxThreads, 1LL), std::min(newN, maxThreads) >>>(d_B, block, step);
				}
			}
			CHECK(cudaMemcpy(temp, d_B, newNBytes, cudaMemcpyDeviceToHost));
			memcpy(gpuRes, temp, nBytes);
			free(temp);

		auto gpuEndTime = cpuSecond();

		// ジャッジ
		// std::sort(h_A, h_A + N);
		bool flag = true;
		rep(i, N)
		{
			if (cpuRes[i] != gpuRes[i])
			{
				flag = false;
				break;
			}
		}
		out << std::fixed << std::setprecision(7);
		out << N << "\t" << (cpuEndTime - cpuStartTime) * 1000 << "ms\t" << (gpuEndTime - gpuStartTime) * 1000 << "ms\t" << (flag ? "Accepted\t" : "Wrong Answer\t") << (cpuEndTime - cpuStartTime) / (gpuEndTime - gpuStartTime) << "\t" << ((cpuEndTime - cpuStartTime) / (gpuEndTime - gpuStartTime) > 1. ? "GPU" : "CPU") << std::endl;
		writing_file << N << "," << (cpuEndTime - cpuStartTime) * 1000 << "," << (gpuEndTime - gpuStartTime) * 1000 << "," << (cpuEndTime - cpuStartTime) / (gpuEndTime - gpuStartTime) << std::endl;

		// 後始末
		free(h_A);
		free(h_B);
		free(cpuRes);
		free(gpuRes);
		cudaFree(d_B);
	}
}
