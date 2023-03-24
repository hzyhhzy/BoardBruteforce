//solve every position of Ataxx with brute force 
#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <set>
#include <array>
#include <cassert>
#include <fstream>
#include <string>

using namespace std;


//~30s if 4x4
//~1h if 4x5
//several days to one month if 5x5
//RAM requirement for 5x5 board: 27GB
const int H = 4;
const int W = 4;
typedef int8_t Color;
const Color C_EMPTY = 0;
const Color C_MY = 1;
const Color C_OPP = 2;
typedef array<Color, H* W> Board;

void printBoard(const Board& board)
{
  static_assert(H <= 9 && W <= 26, "");
  cout << "  ";
  for (int x = 0; x < W; x++)
    cout << char('A' + x) << " ";
  cout << endl;
  for (int y = 0; y < H; y++)
  {
    cout << y + 1 << " ";
    for (int x = 0; x < W; x++)
    {
      Color c = board[y * W + x];
      if (c == C_EMPTY)cout << ". ";
      if (c == C_MY)cout << "x ";
      if (c == C_OPP)cout << "o ";
    }
    cout << endl;
  }
  cout << endl;

}

namespace BoardEncode
{

  // sym8locs
  // encode these 8 locs first
  // -0...1-
  // 2.....3
  // .......
  // .......
  // .......
  // 4.....5
  // -6...7-
  static_assert(H >= 4 && W >= 4, "");
  const int sym8locs[8] = {
    0 * W + 1,
    0 * W + (W - 2),
    1 * W + 0,
    1 * W + (W - 1),
    (H - 2) * W + 0,
    (H - 2) * W + (W - 1),
    (H - 1) * W + 1,
    (H - 1) * W + (W - 2)
  };


  static_assert(H* W <= 39, "");
  const int64_t pow3[40] = {
   1,
   3,
   9,
   27,
   81,
   243,
   729,
   2187,
   6561,
   19683,
   59049,
   177147,
   531441,
   1594323,
   4782969,
   14348907,
   43046721,
   129140163,
   387420489,
   1162261467,
   3486784401,
   10460353203,
   31381059609,
   94143178827,
   282429536481,
   847288609443,
   2541865828329,
   7625597484987,
   22876792454961,
   68630377364883,
   205891132094649,
   617673396283947,
   1853020188851841,
   5559060566555523,
   16677181699666569,
   50031545098999707,
   150094635296999121,
   450283905890997363,
   1350851717672992089,
   4052555153018976267
  };


  //8个位置，3^8种组合，对称方向
  //8种对称，二进制若为abc，则先按c反转x轴，再按b反转y轴，最后按c交换xy
  static int sym8loc_dir[6561];


  //8个位置，3^8种组合，考虑到对称之后的棋形编号
  //总共873种棋形
  static constexpr int sym8typenum = H == W ? 873 : 1701;
  static int sym8loc_encode[6561];

  //8代表8种对称，sym8loc的8个位置为0，其他位置依次是3^0,3^1,...,3^(H*W-8-1)
  static int64_t otherLocEncodeTemplate[8][H * W];






  Board symBoard(const Board& board, int sym)
  {
    Board boardCopy(board);
    if (sym % 2) //x flip
    {
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W / 2; x++)
        {
          int loc1 = y * W + x;
          int loc2 = y * W + (W - 1 - x);
          swap(boardCopy[loc1], boardCopy[loc2]);
        }
    }
    sym /= 2;
    if (sym % 2) //y flip
    {
      for (int y = 0; y < H / 2; y++)
        for (int x = 0; x < W; x++)
        {
          int loc1 = y * W + x;
          int loc2 = (H - 1 - y) * W + x;
          swap(boardCopy[loc1], boardCopy[loc2]);
        }
    }
    if (H != W)return boardCopy;
    sym /= 2;
    if (sym % 2) //x-y transpose
    {
      for (int y = 0; y < H; y++)
        for (int x = 0; x < y; x++)
        {
          int loc1 = y * W + x;
          int loc2 = x * W + y;
          swap(boardCopy[loc1], boardCopy[loc2]);
        }
    }

    return boardCopy;
  }

  int64_t sym8locSimpleEncodeOfBoard(const Board& board)
  {
    int64_t c = 0;
    for (int i = 0; i < 8; i++) {
      c += board[sym8locs[i]] * pow3[i];
    }
    return c;
  }

  static void initSym8locTable()
  {
    map<int, vector<int>> shapeSmallestIdxTable; //map<smallest symmetried encode, all simple encode with the same smallest symmetried encode>
    for (int shape8 = 0; shape8 < 6561; shape8++)
    {
      //init board
      Board baseboard;
      baseboard.fill(C_EMPTY);
      int shape8t = shape8;
      for (int loc8 = 0; loc8 < 8; loc8++)
      {
        int color = shape8t % 3;
        shape8t /= 3;
        baseboard[sym8locs[loc8]] = color;
      }

      //check all symmetries
      int smallestSymDir = -1;
      int smallestSymIdx = 6561;
      for (int sym = 0; sym < 8; sym++)
      {
        Board boardCopy = symBoard(baseboard, sym);
        int simpleEncode = sym8locSimpleEncodeOfBoard(boardCopy);
        if (simpleEncode < smallestSymIdx)
        {
          smallestSymDir = sym;
          smallestSymIdx = simpleEncode;
        }
      }
      //cout << "shape8=" << shape8 << " smallest=" << smallestSymIdx << endl;

      sym8loc_dir[shape8] = smallestSymDir;
      if (shapeSmallestIdxTable.count(smallestSymIdx))
        shapeSmallestIdxTable[smallestSymIdx].push_back(shape8);
      else
      {
        shapeSmallestIdxTable[smallestSymIdx] = vector<int>();
        shapeSmallestIdxTable[smallestSymIdx].push_back(shape8);
      }
    }
    int shapeCount = 0;
    for (auto i = shapeSmallestIdxTable.begin(); i != shapeSmallestIdxTable.end(); i++)
    {
      auto& t = i->second;
      for (int j = 0; j < t.size(); j++)
      {
        sym8loc_encode[t[j]] = shapeCount;
      }
      shapeCount++;
    }
    assert(sym8typenum == shapeCount);
    //this should be 873 if H==W, 1701 if H!=W
    //cout << "sym8loc symmetried shapes: " << shapeCount << endl;
  }

  static void initOtherLocEncodeTemplate()
  {
    array<int64_t, H* W> temp;
    int count = 0;
    for (int loc = 0; loc < H * W; loc++)
    {
      temp[loc] = 0;
      bool isSym8loc = false;
      for (int i = 0; i < 8; i++)
        if (loc == sym8locs[i]) isSym8loc = true;
      if (isSym8loc)continue;
      temp[loc] = pow3[count];
      count++;
    }

    for (int symi = 0; symi < 8; symi++)
    {
      int sym = symi == 5 ? 6 : symi == 6 ? 5 : symi;//逆对称
      auto tempCopy(temp);
      if (sym % 2) //x flip
      {
        for (int y = 0; y < H; y++)
          for (int x = 0; x < W / 2; x++)
          {
            int loc1 = y * W + x;
            int loc2 = y * W + (W - 1 - x);
            swap(tempCopy[loc1], tempCopy[loc2]);
          }
      }
      sym /= 2;
      if (sym % 2) //y flip
      {
        for (int y = 0; y < H / 2; y++)
          for (int x = 0; x < W; x++)
          {
            int loc1 = y * W + x;
            int loc2 = (H - 1 - y) * W + x;
            swap(tempCopy[loc1], tempCopy[loc2]);
          }
      }
      sym /= 2;
      if (H == W && sym % 2) //x-y transpose
      {
        for (int y = 0; y < H; y++)
          for (int x = 0; x < y; x++)
          {
            int loc1 = y * W + x;
            int loc2 = x * W + y;
            swap(tempCopy[loc1], tempCopy[loc2]);
          }
      }

      for (int i = 0; i < H * W; i++)
      {
        otherLocEncodeTemplate[symi][i] = tempCopy[i];
        //cout << tempCopy[i]<<" ";
      }
      //cout << endl;

    }
  }

  static bool selfcheckSym8locTable()
  {
    for (int shape8 = 0; shape8 < 6561; shape8++)
    {
      //init board
      Board baseboard;
      baseboard.fill(C_EMPTY);
      int shape8t = shape8;
      for (int loc8 = 0; loc8 < 8; loc8++)
      {
        int color = shape8t % 3;
        shape8t /= 3;
        baseboard[sym8locs[loc8]] = color;
      }

      {
        int shape8t = sym8locSimpleEncodeOfBoard(baseboard);
        if (shape8t != shape8)
        {
          cout << "shape8t!=shape8 " << shape8t << " " << shape8 << endl;
          return false;
        }
      }


      int shapeid = sym8loc_encode[shape8];

      //check all symmetries
      for (int sym = 0; sym < 8; sym++)
      {
        Board boardCopy = symBoard(baseboard, sym);
        int simpleEncode = sym8locSimpleEncodeOfBoard(boardCopy);
        if (shapeid != sym8loc_encode[simpleEncode])
        {
          cout << "encode not match " << "shape8=" << shape8 << " sym=" << simpleEncode << " encode=" << shapeid << " symencode=" << sym8loc_encode[simpleEncode] << endl;
          return false;
        }
      }

    }
    return true;
  }

}


static int64_t encode(const Board& board)
{
  using namespace BoardEncode;
  int64_t sym8idx = sym8locSimpleEncodeOfBoard(board);
  int64_t sym8encode = sym8loc_encode[sym8idx];
  int64_t sym = sym8loc_dir[sym8idx];

  int64_t x = 0;
  for (int loc = 0; loc < H * W; loc++)
    x += otherLocEncodeTemplate[sym][loc] * board[loc];
  x += sym8encode * pow3[H * W - 8];
  return x;

}

//编号最小的对称方向
static int symDir(const Board& board)
{
  using namespace BoardEncode;
  int64_t sym8idx = sym8locSimpleEncodeOfBoard(board);
  int64_t sym = sym8loc_dir[sym8idx];

  return sym;

}

const int R_WIN = 1;
const int R_LOSE = 2;
const int R_DRAW = 3;
namespace CacheTable
{

  //0=unknown,1=win,2=lose,3=draw
  static vector<uint8_t> table;
  static const int64_t totalSize = BoardEncode::pow3[H * W - 8] * BoardEncode::sym8typenum;
  static const int64_t sizePercent = totalSize / 100 + 1;
  static int64_t setCount = 0;
  static void init()
  {
    cout << "Position num=" << totalSize << endl;
    int64_t tableLen = (totalSize + 4) / 4;
    table.resize(tableLen);
    fill(table.begin(), table.end(), 0);
  }
  static bool save(string path);
  static void set(int64_t key, uint8_t value)
  {
    setCount++;
    if (setCount % sizePercent == 0)
    {
      cout << setCount / sizePercent << "% finished" << endl;
      //string cachePath = to_string(H) + "x" + to_string(W) + "_backup.ataxx";
      //save(cachePath);
    }
    assert(value < 4);
    int64_t idx = key / 4;
    int p = key % 4;
    uint8_t oldv = table[idx];
    oldv &= ~(uint8_t(0b11) << (p * 2));
    oldv |= (value << (p * 2));
    table[idx] = oldv;
  }
  static int get(int64_t key)
  {
    int64_t idx = key / 4;
    int p = key % 4;
    uint8_t v = table[idx];
    v = v >> (p * 2);
    v = v & 0b11;
    return v;
  }
  static bool save(string path)
  {
    cout << "Saving cache: " << path << "...";
    std::ofstream cacheStream(path, std::ios::binary);
    if (!cacheStream.good()) {
      return false;
    }
    cout << "done" << endl;
    cacheStream.write(reinterpret_cast<char*>(table.data()), sizeof(uint8_t) * table.size());
    return true;
  }
  static bool load(string path)
  {
    cout << "Loading cache: " << path << "...";
    std::ifstream cacheStream(path, std::ios::binary);
    if (!cacheStream.good()) {
      cout << "No cache" << endl;
      return false;
    }
    cacheStream.read(reinterpret_cast<char*>(table.data()), sizeof(uint8_t) * table.size());
    if (cacheStream.good()) {
      cout << "done" << endl;
      return true;
    }
    else
    {
      cout << "Bad cache file: " << path << endl;
      fill(table.begin(), table.end(), 0);
      return false;
    }
  }
}

static void initAll()
{
  using namespace BoardEncode;
  initSym8locTable();
  initOtherLocEncodeTemplate();
  selfcheckSym8locTable();
  CacheTable::init();
}

Board inverseBoard(const Board& board)
{
  Board invBoard;
  for (int i = 0; i < H * W; i++)
  {
    Color c = board[i];
    if (c == 1)c = 2;
    else if (c == 2)c = 1;
    invBoard[i] = c;
  }
  return invBoard;
}

//loc1是选的子，如果是复制棋子则为-1
Board playMove(const Board& board, int loc1, int loc2, Color pla)
{

  Board b = board;

  if (loc1 >= 0)
    b[loc1] = C_EMPTY;

  int x0 = loc2 % W, y0 = loc2 / W;
  for (int x = max(0, x0 - 1); x <= min(W - 1, x0 + 1); x++)
    for (int y = max(0, y0 - 1); y <= min(H - 1, y0 + 1); y++)
    {
      int loc = y * W + x;
      if (b[loc] != C_EMPTY)
        b[loc] = pla;
    }
  b[loc2] = pla;

  return b;
}



vector<Board> allNextBoardInverse(const Board& board)
{
  Board invBoard = inverseBoard(board);
  vector<Board> allBoard;

  //copy stone
  for (int x0 = 0; x0 < W; x0++)
    for (int y0 = 0; y0 < H; y0++)
    {
      int loc0 = y0 * W + x0;
      if (board[loc0] != C_EMPTY)
        continue;

      bool isLegal = false;
      for (int x = max(0, x0 - 1); x <= min(W - 1, x0 + 1); x++)
      {
        for (int y = max(0, y0 - 1); y <= min(H - 1, y0 + 1); y++)
        {
          if (x == x0 && y == y0)
            continue;
          int loc = y * W + x;
          if (invBoard[loc] == C_OPP)
            isLegal = true;
          if (isLegal)break;
        }
        if (isLegal)break;
      }
      if (isLegal)
        allBoard.push_back(playMove(invBoard, -1, loc0, C_OPP));

    }
  //jump stone
  for (int x0 = 0; x0 < W; x0++)
    for (int y0 = 0; y0 < H; y0++)
    {
      int loc0 = y0 * W + x0;
      if (board[loc0] == C_EMPTY)
      {
        for (int x = max(0, x0 - 2); x <= min(W - 1, x0 + 2); x++)
          for (int y = max(0, y0 - 2); y <= min(H - 1, y0 + 2); y++)
          {
            int loc = y * W + x;
            if (invBoard[loc] == C_OPP)
            {
              int distSqr = (x - x0) * (x - x0) + (y - y0) * (y - y0);
              if (distSqr >= 4)
              {
                allBoard.push_back(playMove(invBoard, loc, loc0, C_OPP));
              }
            }
          }
      }
    }
  return allBoard;
}

int calculateDepth1(const Board& board)
{
  bool isUncertain = false;
  int bestResult = R_LOSE;
  auto allNextBoard = allNextBoardInverse(board);
  if (allNextBoard.size() == 0)//finish
  {
    int score = 0;
    for (int i = 0; i < H * W; i++)
    {
      if (board[i] == C_MY)score += 1;
      else score -= 1;
    }
    if (score > 0)bestResult = R_WIN;
    else if (score < 0)bestResult = R_LOSE;
    else bestResult = R_DRAW;
  }
  else
  {
    for (auto i = allNextBoard.begin(); i != allNextBoard.end(); i++)
    {
      int r = CacheTable::get(encode(*i));
      if (r == R_LOSE)//win
      {
        bestResult = R_WIN;
        isUncertain = false;
        break;
      }
      else if (r == R_DRAW)//draw
      {
        bestResult = R_DRAW;
      }
      else if (r == 0)
        isUncertain = true;
    }
  }

  if (isUncertain)
    return 0;

  int64_t hash = encode(board);


  int r2 = CacheTable::get(hash);
  if (r2 != 0 && r2 != bestResult)
  {
    cout << "ERROR result not match, search result=" << bestResult << ", cache result=" << r2 << endl;
    printBoard(board);
  }
  if (r2 == 0)
    CacheTable::set(hash, bestResult);
  return bestResult;
}

Board findBestNextBoard(const Board& board)
{
  auto allNextBoard = allNextBoardInverse(board);
  if (allNextBoard.size() == 0)//finish
  {
    return board;
  }
  else
  {
    Board bestBoard = inverseBoard(allNextBoard[0]);
    for (auto i = allNextBoard.begin(); i != allNextBoard.end(); i++)
    {
      int r = calculateDepth1(*i);
      if (r == R_LOSE)//win
      {
        return inverseBoard(*i);
      }
      if (r == R_DRAW)//draw
      {
        bestBoard = inverseBoard(*i);
      }
    }
    return bestBoard;
  }
}


static void calculateAll()
{
  Board board;


  bool hasCheckpoint = false;
  int checkpoint_stonenum = H * W;
  int checkpoint_movenum = 0;
  string checkpointName = to_string(H) + "x" + to_string(W) + "_checkpoint";
  ifstream checkpointLogFile(checkpointName + ".txt");

  if (checkpointLogFile.good())
  {
    checkpointLogFile >> checkpoint_stonenum >> checkpoint_movenum;
    if (checkpointLogFile.good() && checkpoint_stonenum >= 0 && checkpoint_stonenum <= H * W && checkpoint_movenum > 0 && checkpoint_movenum <= 10000)
    {
      if (CacheTable::load(checkpointName + ".ataxx"))
      {
        hasCheckpoint = true;
      }
    }
  }

  if (!hasCheckpoint)
  {
    checkpoint_stonenum = H * W;
    checkpoint_movenum = 0;
  }
  else cout << "Loaded checkpoint: stonenum=" << checkpoint_stonenum << " movenum= " << checkpoint_movenum << endl;



  for (int stonenum = checkpoint_stonenum; stonenum >= 0; stonenum--)
  {
    for (int movenum = stonenum == checkpoint_stonenum ? checkpoint_movenum + 1 : 1;; movenum++)
    {
      int64_t newResult = 0;
      //穷举所有棋盘，使用类似三进制进位的方式
      for (int loc = 0; loc < H * W; loc++)
      {
        board[loc] = 0;
      }
      while (1)
      {
        if (symDir(board) == 0)//所有对称局面只考虑一种
        {
          int s = 0;
          for (int loc = 0; loc < H * W; loc++)
          {
            if (board[loc] != C_EMPTY)s++;
          }

          if (s == stonenum)
          {
            int r = CacheTable::get(encode(board));
            if (r == 0)
            {
              r = calculateDepth1(board);
              if (r != 0)
                newResult++;
            }
          }
        }

        //使用类似三进制进位的方式
        board[0] += 1;
        for (int i = 0; i < H * W - 1; i++)
        {
          if (board[i] >= 3)
          {
            board[i] = 0;
            board[i + 1] += 1;
          }
          else break;
        }
        if (board[H * W - 1] >= 3)
          break;
      }
      cout << "stonenum=" << stonenum << ", step=" << movenum << ", newResults=" << newResult << endl;
      if (newResult == 0)
        break;

      ofstream checkpointLogFile(checkpointName + ".txt");
      if (checkpointLogFile.good())
      {
        CacheTable::save(checkpointName + ".ataxx");
        checkpointLogFile << stonenum << " " << movenum << "  ";
      }
    }


    //剩下的局面都是循环导致的和棋
    int64_t drawnum = 0;
    //穷举所有棋盘，使用类似三进制进位的方式
    for (int loc = 0; loc < H * W; loc++)
    {
      board[loc] = 0;
    }
    while (1)
    {
      int s = 0;
      for (int loc = 0; loc < H * W; loc++)
      {
        if (board[loc] != C_EMPTY)s++;
      }

      if (s == stonenum)
      {
        int h = encode(board);
        int r = CacheTable::get(h);
        if (r == 0)
        {
          if (drawnum == 0)
          {
            cout << "Loop draw example" << endl;
            printBoard(board);
          }
          drawnum++;
          CacheTable::set(h, R_DRAW);
        }
      }

      //使用类似三进制进位的方式
      board[0] += 1;
      for (int i = 0; i < H * W - 1; i++)
      {
        if (board[i] >= 3)
        {
          board[i] = 0;
          board[i + 1] += 1;
        }
        else break;
      }
      if (board[H * W - 1] >= 3)
        break;
    }
    cout << "stonenum=" << stonenum << ", loopDraws=" << drawnum << endl;

  }


}


int main()
{
  initAll();
  string cachePath = to_string(H) + "x" + to_string(W) + ".ataxx";
  bool hasCache = CacheTable::load(cachePath);
  if (!hasCache)
  {
    calculateAll();
    CacheTable::save(cachePath);
  }



  Board board;
  for (int loc = 0; loc < H * W; loc++)
  {
    board[loc] = 0;
  }
  board[0] = C_MY;
  board[H * W - 1] = C_MY;
  board[W - 1] = C_OPP;
  board[H * W - W] = C_OPP;



  while (true)
  {
    //cout << "Your moves:" << endl;

    //auto allBoard = allNextBoardInverse(board);
    //for (int i = 0; i < allBoard.size(); i++)
    //{
    //  Board b = allBoard[i];
    //  cout << "Move " << i << ", Result = " << calculateDepth1(b) << endl;
    //  printBoard(b);
    //  cout << endl;
    //}

    cout << "Current board:" << endl;
    printBoard(board);

    string playerMove;
    cin >> playerMove;
    if (playerMove.size() == 2)
    {
      int x = playerMove[0] - 'a';
      int y = playerMove[1] - '1';
      if (x < 0 || x >= W || y < 0 || y >= H)
        continue;
      board = playMove(board, -1, y * W + x, C_MY);
    }
    else if (playerMove.size() == 4)
    {
      int x1 = playerMove[0] - 'a';
      int y1 = playerMove[1] - '1';
      int x2 = playerMove[2] - 'a';
      int y2 = playerMove[3] - '1';
      if (x1 < 0 || x1 >= W || y1 < 0 || y1 >= H)
        continue;
      if (x2 < 0 || x2 >= W || y2 < 0 || y2 >= H)
        continue;
      board = playMove(board, y1 * W + x1, y2 * W + x2, C_MY);
    }
    else continue;

    printBoard(board);

    board = inverseBoard(board);
    int res = calculateDepth1(board);
    string resultStr =
      res == 0 ? "Unknown" : 
      res == R_WIN ? "Win" : 
      res == R_DRAW ? "Draw" : 
      res == R_LOSE ? "Lose" : 
      "ERROR";
    cout << "Bot Result=" + resultStr << endl;
    board = findBestNextBoard(board);
    board = inverseBoard(board);
  }
  return 0;
}
