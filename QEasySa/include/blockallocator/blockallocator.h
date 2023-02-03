#pragma execution_character_set("utf-8")
/*
* Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __BLOCKALLOCATOR_H__
#define __BLOCKALLOCATOR_H__

#include <cstdint>

/// This is a small object allocator used for allocating small
/// objects that persist for more than one time step.
/// See: http://www.codeproject.com/useritems/Small_Block_Allocator.asp
constexpr int kChunkSize = 16 * 1024;
constexpr int kMaxBlockSize = 640;
constexpr int kBlockSizes = 14;
constexpr int kChunkArrayIncrement = 128;

typedef struct tagBlock
{
	tagBlock* next;
}Block;

typedef struct tagChunk
{
	int block_size;
	Block* blocks;
}Chunk;



class BlockAllocator
{
public:
	BlockAllocator();
	~BlockAllocator();

public:
	void* allocate(int size);
	void free(void* p, int size);
	void clear();

private:
	int             num_chunk_count_ = 0;
	int             num_chunk_space_ = 0;
	Chunk* chunks_ = nullptr;
	Block* free_lists_[kBlockSizes] = { nullptr };

	//static int      block_sizes_[kBlockSizes];
	uint8_t  s_block_size_lookup_[kMaxBlockSize + 1] = {};
	bool     s_block_size_lookup_initialized_ = false;
};

#endif
