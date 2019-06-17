﻿/*
777 游戏规则

格子: 坐标码( 数组下标 )
 0  1  2  3  4
 5  6  7  8  9
10 11 12 13 14

判定线下标列表：
5, 6, 7, 8, 9
0, 1, 2, 3, 4
10, 11, 12, 13, 14
0, 6, 12, 8, 4
10, 6, 2, 8, 14
0, 1, 7, 3, 4
10, 11, 7, 13, 14
5, 11, 12, 13, 9
5, 1, 2, 3, 9

中奖规则：
依据判定线下标 正反双向扫格子。从第 1 格开始数，连续出现相同形状 3 次或以上，即中奖，金额查表.
0 符号可替代任意非 0 符号, 计数时要考虑进去.
多条判定线可重复中奖. 最终奖励叠加.
另外, 存在所有格子同时判定的情况，比如全部都是相同符号或符合某种分类

符号列表:
0, 1, 2, 3, ......9

奖励表:
.......
0*3 特殊1
0*4 特殊2
0*5 特殊3
1*3 5倍
1*4 10倍
1*5 20倍
2*3 10倍
2*4 20倍
2*5 40倍
......
1*15 1000倍
2*15 2000倍
......
单数*15 500倍
双数*15 500倍

需求：
对所有格子随机填充符号, 统计出最终倍数
*/

#pragma execution_character_set("utf-8")

#include <iostream>
#include <array>
#include <random>
#include <cassert>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <string>
#include <unordered_set>
#include <xx_file.h>

struct Result {
	int index;			// 线下标. -1 代表全屏
	int direction;		// 方向( 0: 从左到右. 1: 从右到左 )
	int symbol;			// 中奖符号
	int count;			// 符号个数
	void Dump() {
		std::cout << index << (direction ? " <-- " : " --> ") << symbol << "*" << count << std::endl;
	}
};

const int gridLen = 15;
const int lineLen = 5;
const int linesLen = 9;
using Grid = std::array<int, gridLen>;
using Line = std::array<int, lineLen>;
using Lines = std::array<Line, linesLen>;
using Results = std::array<Result, linesLen * 2>;


//inline std::mt19937_64 rnd;
//inline std::uniform_int_distribution gen(0, numSymbols - 1);
//rnd.seed(std::random_device()());
//inline void Fill() {
//	for (auto&& v : grid) {
//		v = gen(rnd);
//	}
//}

inline void Dump(Grid const& grid) {
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 5; ++i) {
			auto&& v = grid[j * 5 + i];
			if (i) {
				std::cout << " ";
			}
			if (v == -1) {
				std::cout << "_";
			}
			else {
				std::cout << v;
			}
		}
		std::cout << std::endl;
	}
}

template<int offset, int step = (offset > 0 ? 1 : -1)>
inline void CalcLine(int& symbol, int& count, int const* cursor, Grid const& grid) noexcept {
	symbol = grid[*cursor];
	count = 1;
	auto end = cursor + offset;
	// 如果 0 打头, 向后找出一个非 0 的来...
	if (!symbol) {
		while ((cursor += step) != end) {
			if ((symbol = grid[*cursor])) break;
			++count;
		}
		// 3 个 0 优先判定
		if (count >= 3) {
			symbol = 0;
			return;
		}
		++count;
	}
	// 继续数当前符号个数
	while ((cursor += step) != end) {
		int s = grid[*cursor];
		if (s && symbol != s) break;
		++count;
	}
}

inline void CalcGrid(int& symbol, int& count, Grid const& grid) noexcept {
	int cursor = 0;
	symbol = grid[cursor];
	count = 1;
	const auto end = grid.size();
	// 如果 0 打头, 向后找出一个非 0 的来...
	if (!symbol) {
		while (++cursor != end) {
			if ((symbol = grid[cursor])) break;
			++count;
		}
		if (cursor == end) return;
		++count;
	}
	// 继续数当前符号个数
	while (++cursor != end) {
		int s = grid[cursor];
		if (s && symbol != s) break;
		++count;
	}
}

inline void Calc(Result* results, int& resultsLen, Grid const& grid, Lines const& lines) noexcept {
	int s1, c1, s2, c2;
	CalcGrid(s1, c1, grid);
	if (c1 == 15) {
		auto&& r = results[resultsLen++];
		r.index = -1;
		r.direction = 0;
		r.symbol = s1;
		r.count = c1;
		return;
	}
	// 这里不需要总类全屏特殊判断
	for (size_t i = 0; i < linesLen; ++i) {
		CalcLine<lineLen>(s1, c1, (int*)& lines[i], grid);
		CalcLine<-lineLen>(s2, c2, (int*)& lines[i] + lineLen - 1, grid);
		if (c1 >= 3) {
			auto&& r = results[resultsLen++];
			r.index = (int)i;
			r.direction = 0;
			r.symbol = s1;
			r.count = c1;
		}
		if (c2 >= 3 && (s1 != s2/* && c1 != c2*/)) {
			auto&& r = results[resultsLen++];
			r.index = (int)i;
			r.direction = 1;
			r.symbol = s2;
			r.count = c2;
		}
	}
}

union GridValues {
	struct {
		uint8_t c0 : 2;
		uint8_t c1 : 2;
		uint8_t c2 : 2;
		uint8_t c3 : 2;
		uint8_t c4 : 2;
		uint8_t c5 : 2;
		uint8_t c6 : 2;
		uint8_t c7 : 2;
		uint8_t c8 : 2;
		uint8_t c9 : 2;
		uint8_t c10 : 2;
		uint8_t c11 : 2;
		uint8_t c12 : 2;
		uint8_t c13 : 2;
		uint8_t c14 : 2;
		uint8_t c__ : 2;
	};
	uint32_t value;
};

union ShapeNums {
	struct {
		uint8_t n3 : 8;
		uint8_t n4 : 8;
		uint8_t n5 : 8;
		uint8_t n15 : 8;
	};
	uint32_t value;
};

union LineMask {
	struct {
		uint8_t index : 4;
		uint8_t direction : 1;
		uint8_t count : 3;
	};
	char value;
};

// 返回二进制的指定线上的 0 掩码
template<int offset, int step = (offset > 0 ? 1 : -1)>
inline int CalcLineZeroMask(int const& count, int const* const& line, Grid const& grid) noexcept {
	int rtv = 0;
	for (int i = 0; i < count; ++i) {
		if (!grid[*(line + i * step)]) {
			rtv |= (1 << i);
		}
	}
	return rtv;
}

// 返回二进制的 0 分布图
inline int CalcGridZeroMask(Grid const& grid) noexcept {
	int rtv = 0;
	for (int i = 0; i < grid.size(); ++i) {
		if (!grid[i]) {
			rtv |= (1 << i);
		}
	}
	return rtv;
}


// 返回 0 的个数
inline size_t CalcGridZeroCount(Grid const& grid) noexcept {
	size_t rtv = 0;
	for (int i = 0; i < grid.size(); ++i) {
		if (!grid[i]) {
			++rtv;
		}
	}
	return rtv;
}

// 返回 0 的平均出现个数( 结果为 3.75 )
inline void CalcAvgZeroCount() noexcept {
	Grid grid;
	GridValues g;
	size_t count = 0;
	size_t count2 = 0;
	for (g.value = 0; g.value <= 0b111111111111111111111111111111; ++g.value) {
		grid = {
			g.c0, g.c1, g.c2, g.c3, g.c4,
			g.c5, g.c6, g.c7, g.c8, g.c9,
			g.c10, g.c11, g.c12, g.c13, g.c14,
		};
		auto&& n = CalcGridZeroCount(grid);
		if (n) {
			count += n;
			++count2;
		}
	}
	std::cout << (double)count / 0b111111111111111111111111111111 << ", " << count2;
}

//int main() {
//	CalcAvgZeroCount();
//	return 0;
//}




//int main() {
//	Lines lines = {
//		5, 6, 7, 8, 9,
//		0, 1, 2, 3, 4,
//		10, 11, 12, 13, 14,
//		0, 6, 12, 8, 4,
//		10, 6, 2, 8, 14,
//		0, 1, 7, 3, 4,
//		10, 11, 7, 13, 14,
//		5, 11, 12, 13, 9,
//		5, 1, 2, 3, 9,
//	};
//	Grid grid;
//	std::array<int, 4> counts;
//	std::unordered_map<uint32_t, int> shapes;
//	std::unordered_map<uint32_t, std::unordered_set<std::string>> shapesResults;
//	Results results;
//	int resultsLen = 0;
//	GridValues g;
//	for (g.value = 0; g.value <= 0b111111111111111111111111111111; ++g.value) {
//		if ((g.value & 0b11111111111111111111111) == 0) {
//			std::cout << ".";
//			std::cout.flush();
//		}
//		grid = {
//			g.c0, g.c1, g.c2, g.c3, g.c4,
//			g.c5, g.c6, g.c7, g.c8, g.c9,
//			g.c10, g.c11, g.c12, g.c13, g.c14,
//		};
//		resultsLen = 0;
//		Calc((Result*)& results, resultsLen, grid, lines);
//		if (!resultsLen) continue;
//		if (results[0].index == -1) continue;		// ignore full screen
//		
//		counts.fill(0);
//		std::string masks;
//		auto zeroMask = (uint16_t)CalcGridZeroMask(grid);
//		masks.append((char*)& zeroMask, sizeof(zeroMask));
//		for (int i = 0; i < resultsLen; ++i) {
//			++counts[results[i].count - 3];
//			LineMask m;
//			m.index = results[i].index;
//			m.direction = results[i].direction;
//			m.count = results[i].count;
//			masks.append((char*)&m.value, sizeof(m.value));
//		}
//
//		auto key = counts[0] | counts[1] << 8 | counts[2] << 16;
//		++shapes[key];
//
//		shapesResults[key].insert(std::move(masks));
//	}
//
//	std::cout << shapes.size() << std::endl;
//
//	size_t siz = 0;
//	ShapeNums n;
//	std::string s;
//	for (auto&& iter : shapes) {
//		n.value = iter.first;
//		s.clear();
//		if (n.n3) {
//			s.append("3*");
//			s.append(std::to_string(n.n3));
//		}
//		if (n.n4) {
//			if (s.size()) {
//				s.append(", ");
//			}
//			s.append("4*");
//			s.append(std::to_string(n.n4));
//		}
//		if (n.n5) {
//			if (s.size()) {
//				s.append(", ");
//			}
//			s.append("5*");
//			s.append(std::to_string(n.n5));
//		}
//		if (n.n15) {
//			if (s.size()) {
//				s.append(", ");
//			}
//			s.append("15*");
//			s.append(std::to_string(n.n5));
//		}
//		auto&& numResults = shapesResults[iter.first].size();
//		siz += numResults;
//		s = s + " : " + std::to_string(iter.second) + ", shapesResults.size() = " + std::to_string(numResults);
//		std::cout << s << std::endl;
//
//
//	}
//	std::cout << "total shapesResults 's size = " << siz << std::endl;
//	
//	xx::BBuffer bb;
//	for (auto&& iter : shapesResults) {
//		bb.Write(iter.first);				// 3x4y5z
//		bb.Write(iter.second.size());		// len
//		for (auto&& str : iter.second) {	// masks( grid zero mask 2 bytes + LineMask 1 byte * n
//			bb.Write(str);
//		}
//	}
//	xx::WriteAllBytes("huga.bin", bb);
//	std::cout << "bb.len = " << bb.len << std::endl;		// 最终输出 482131 KB
//}

inline void FillGridByZeroMask(Grid& grid, std::string const& s) {
	assert(s.size() > 2);
	auto mask = *(uint16_t*)s.data();
	xx::CoutN(mask);
	for (int i = 0; i < 15; ++i) {
		if (mask & (1 << i)) {
			grid[i] = 0;
		}
	}
}

int main() {
	xx::BBuffer bb;
	xx::ReadAllBytes("huga.bin", bb);
	xx::CoutN(bb.len);

	// todo: 还原成字典 并填充权值啥的 准备随机函数 试着随机并填充牌型. 0 直接按照 mask 来填充
	std::vector<std::pair<uint32_t, std::vector<std::string>>> shapes;
	size_t len;
	while (bb.offset < bb.len) {
		auto&& ss = shapes.emplace_back();
		if (int r = bb.Read(ss.first)) return r;
		if (int r = bb.Read(len)) return r;
		ss.second.reserve(len);
		for (size_t i = 0; i < len; ++i) {
			auto&& s = ss.second.emplace_back();
			if (int r = bb.Read(s)) return r;
		}
	}
	xx::CoutN("shapes.len = ", shapes.size());
	ShapeNums n;
	std::string s;
	for (int i = 0; i < shapes.size(); ++i) {
		auto&& pair = shapes[i];
		n.value = pair.first;
		s.clear();
		if (n.n3) {
			s.append("3*");
			s.append(std::to_string(n.n3));
		}
		if (n.n4) {
			if (s.size()) {
				s.append(", ");
			}
			s.append("4*");
			s.append(std::to_string(n.n4));
		}
		if (n.n5) {
			if (s.size()) {
				s.append(", ");
			}
			s.append("5*");
			s.append(std::to_string(n.n5));
		}
		if (n.n15) {
			if (s.size()) {
				s.append(", ");
			}
			s.append("15*");
			s.append(std::to_string(n.n5));
		}
		s = s + " : " + std::to_string(pair.second.size());
		std::cout << "shapes[" << i << "] = " << s << std::endl;
	}

	// 试定位到某 shape 某 grid zero mask
	auto&& ss = shapes[25].second;
	Grid g;
	for (auto&& s : ss) {
		g.fill(-1);
		FillGridByZeroMask(g, s);
		Dump(g);
		xx::CoutN();
	}

	// 填充思路：先填所有 0. 然后 foreach 每线。 每条线的中奖位里面，如果有空位置，就随机一个非 0 填上. 
	// 如果 有空位但是存在非0， 则用非0 继续填充 ( 避免交叉格子冲突 )
	// 最后填充杂物. 
	// 杂物填充大体思路：排除掉 0 和所有线用掉的符号，剩下的交错出现以避免形成新的中奖线条
	// todo: 杂物填充细节. 总的来说用排除法. 避免和邻居相同

	return 0;
}




/*

..... 约 993 种 shape + 全屏

*/



//int Test() {//inline std::array<std::array<int, 5>, 1> lines;
	//lines[0] = { 0, 1, 2, 3, 4 };
//	Init();
//	results.reserve(lines.size() * 2);
//	{
//		grid = { 2, 0, 0, 0, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 2, 0, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 2, 2, 0, 0, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 2, 2, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 2, 2, 2, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 1, 1, 1, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(1, 3));
//	}
//	{
//		grid = { 1, 1, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(1, 3) && results[1] == std::make_pair(2, 3));
//	}
//	{
//		grid = { 1, 0, 1, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(1, 3));
//	}
//	{
//		grid = { 1, 0, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(1, 3) && results[1] == std::make_pair(2, 4));
//	}
//	{
//		grid = { 0, 1, 1, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(1, 3));
//	}
//	{
//		grid = { 0, 1, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(1, 3) && results[1] == std::make_pair(2, 3));
//	}
//	{
//		grid = { 0, 0, 1, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(1, 3));
//	}
//	{
//		grid = { 0, 0, 0, 2, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(0, 3) && results[1] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 0, 0, 0, 0, 2 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(0, 4) && results[1] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 0, 0, 0, 2, 0 };
//		results.clear();
//		Calc();
//		assert(results.size() == 2 && results[0] == std::make_pair(0, 3) && results[1] == std::make_pair(2, 5));
//	}
//	{
//		grid = { 0, 0, 0, 0, 0 };
//		results.clear();
//		Calc();
//		assert(results.size() == 1 && results[0] == std::make_pair(0, 5));
//	}
//
//	return 0;
//}
