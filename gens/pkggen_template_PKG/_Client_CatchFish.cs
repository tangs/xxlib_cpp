﻿#pragma warning disable 0169, 0414
using TemplateLibrary;

// Client -> Server
namespace Client_CatchFish
{
    [Desc("申请进入游戏. 成功返回 EnterSuccess. 失败直接被 T")]
    class Enter
    {
        [Desc("传递先前保存的玩家id以便断线重连. 没有传 0")]
        int playerId;
    }

    [Desc("开火")]
    class Fire
    {
        int frameNumber;
        int cannonId;
        int bulletId;
        xx.Pos pos;
    }

    [Desc("碰撞检测")]
    class Hit
    {
        int cannonId;
        int bulletId;
        int fishId;
    }
}
