﻿
#include <iostream>
#include <chrono>
#include <type_traits>
#include <algorithm>
#include <memory>
#include <cmath>
#include "file.hpp"
#include "astar.hpp"

using namespace moon;

static int64_t ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct user_context
{

    int x = 0;
    int y = 0;
    int t = 0;

    float estimate(const user_context& goal)
    {
        //return 0;
        return sqrtf(float((x - goal.x) * (x - goal.x) + (y - goal.y) * (y - goal.y)));
        //return (std::abs(x - goal.x) + std::abs(y - goal.y));
    }

    bool canpass()
    {
        return t == 1;
    }
};

using graph_t = graph<astar_policy, user_context>;
using astar_t = astar< graph_t>;

std::unique_ptr<graph_t> make_graph(int M, int N, const std::string& obs)
{
    auto m = std::make_unique<graph_t>(M, N);
    for (int j = 0; j < N; ++j)
    {
        for (int i = 0; i < M; ++i)
        {
            auto vtx = m->at(i, j);
            vtx->context.x = i;
            vtx->context.y = j;
            vtx->context.t = obs[(N)*j + i] == '%' ? 0 : 1;
        }
    }
    return m;
}

void print_path(const astar_t& a, graph_t* g2)
{
    std::vector< std::pair<int, int>> pathpos;
    auto v = a.start();
    do {
        v = (astar_t::vertex_type*)v->child;
        pathpos.emplace_back(v->context.x, v->context.y);
    } while (v->child != nullptr);

    std::string filename = "path.txt";

    std::ofstream outfile(filename.c_str(), std::ofstream::binary);
    for (int y = 0; y != g2->height(); ++y)
    {
        for (int x = 0; x != g2->width(); ++x)
        {
            std::pair<int, int> id{ x, y };
            //outfile << std::left << std::setw(1);
            if (std::find(pathpos.begin(), pathpos.end(), id) != pathpos.end())
                outfile << '@';
            else if (g2->at(x, y)->context.canpass())
            {
                outfile << ' ';
            }
            else
                outfile << '%';
        }
        outfile << '\n';
    }
}

void _search(graph_t* g2, const user_context& from, const user_context& to)
{
    auto bt = ms();

    int count = 1000;
    int success = 0;

    astar_t a;
    for (int i = 0; i < count; ++i)
    {
        a.init(g2, from, to);
        do
        {
            auto st = a.search();
            if (st == astar_t::searching)
            {
                continue;
            }
            else if (st == astar_t::succeeded)
            {
                success++;

                //print_path(a, g2);
                break;
            }
            else
            {
                printf("failed");
                return;
            }
        } while (true);
    }

    printf("total cost %lld ms\n", ms() - bt);
    printf("success %d.\n", success);
}

int main(int argc, char* argv[])
{
    std::string grids = moon::file::read_all("map2.txt", std::ios::binary | std::ios::in);
    std::remove_if(grids.begin(), grids.end(), [](char c) {
        return c == '\n' || c == '\r';
        });

    {
        auto bt = ms();
        auto g2 = make_graph(100, 100, grids);
        printf("make graph cost %lld ms\n", ms() - bt);
        _search(g2.get(), user_context{ 3,2 }, user_context{ 96,97 });
    }
    return 0;
}


//#include "xx_epoll2_http.h"
//namespace EP = xx::Epoll;
//
//void Bind(EP::Context& ep, EP::Ref<EP::HttpListener> const& listener) {
//	listener->handlers[""] = [&](xx::HttpContext& q, xx::HttpResponse& r) {
//		char const str[] = R"--(
//<html><body>
//<a href="exit">exit</p>
//</body></html>
//)--";
//		r.onSend(r.prefixHtml, str, sizeof(str));
//	};
//	listener->handlers["exit"] = [&](xx::HttpContext& q, xx::HttpResponse& r) {
//		ep.running = false;
//	};
//
//	ep.cmds["exit"] = [&](auto args) { ep.running = false; };
//	ep.cmds["quit"] = ep.cmds["exit"];
//}
//
//
//int main() {
//	xx::IgnoreSignal();
//	EP::Context ep;
//	auto&& listener = ep.CreateTcpListener<EP::HttpListener>(12312);
//	if (!listener) return -1;
//	Bind(ep, listener);
//	ep.Run(100);
//	return 0;
//}
