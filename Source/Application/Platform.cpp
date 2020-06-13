//	VQE
//	Copyright(C) 2020  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#define NOMINMAX

#include "Platform.h"
#include "Window.h"
#include "../Renderer/Renderer.h" // GetDX12Adapters

#include <dxgi1_6.h>

FSystemInfo FSystemInfo::GetSystemInfo()
{
	FSystemInfo inf = {};

	// GPU 
	inf.GPUInfoVec = std::move(VQRenderer::EnumerateDX12Adapters(false, false));
	
	// MONITOR
	// TODO

	// CPU
	// TODO

	// RAM
	// TODO

	return inf;
}

void Semaphore::Wait()
{
	std::unique_lock<std::mutex> lk(mtx);
	cv.wait(lk, [&]() {return currVal > 0; });
	--currVal;
	return;
}

void Semaphore::Signal()
{
	std::unique_lock<std::mutex> lk(mtx);
	currVal = std::min(currVal+1, maxVal);
	cv.notify_one();
}
