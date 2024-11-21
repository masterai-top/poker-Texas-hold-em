#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <wbl/regex_util.h>
#include <wbl/pthread_util.h>
#include "OuterFactory.h"

using namespace std;
using namespace tars;
using namespace wbl;

class WordTree;

class WordNode
{
private:
    //
    friend class WordTree;
    //
    typedef map<string, WordNode> _TreeMap;
    //
    typedef map<string, WordNode>::iterator _TreeMapIterator;
    //
    string m_character;
    //
    _TreeMap m_map;
    //
    WordNode *m_parent;

public:
    //
    WordNode(string character);
    //
    WordNode()
    {
        m_character = "";
    }
    //
    string getCharacter() const
    {
        return m_character;
    }
    //
    WordNode *findChild(string &nextCharacter);
    //
    WordNode *insertChild(string &nextCharacter);
};

class WordTree
{
public:
    //
    int count;
    //
    WordNode *insert(string &keyword);
    //
    WordNode *insert(const char *keyword);
    //
    WordNode *find(string &keyword);
    //
    WordTree()
    {
        count = 0;
    }

private:
    //
    WordNode m_emptyRoot;
    //
    int m_pace;
    //
    WordNode *insert(WordNode *parent, string &keyword);
    //
    WordNode *insertBranch(WordNode *parent, string &keyword);
    //
    WordNode *find(WordNode *parent, string &keyword);
};

class WordFilter
{
public:
    //
    static WordFilter *pInstace;
    //
    static WordFilter *sharedInstace();
    //
    static void release();

private:
    //
    WordTree m_tree;
    //
    wbl::ReadWriteLocker m_rwlock;

public:
    //
    void load(const char *filepath);
    //
    void load(const vector<string> &words);
    //
    bool censor(string &source);
};
