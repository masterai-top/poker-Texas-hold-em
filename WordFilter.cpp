#include "WordFilter.h"
#include "LogComm.h"

#define PACE 1

WordFilter *WordFilter::pInstace = NULL;

WordNode::WordNode(string character)
{
    if (character.size() == PACE)
        m_character.assign(character);
    else
        cout << "error" << endl;
}

WordNode *WordNode::findChild(string &nextCharacter)
{
    auto TreeMapIt = m_map.find(nextCharacter);
    if (TreeMapIt == m_map.end())
        return NULL;

    return &TreeMapIt->second;
}

WordNode *WordNode::insertChild(string &nextCharacter)
{
    if (!findChild(nextCharacter))
    {
        m_map.insert(pair<string, WordNode>(nextCharacter, WordNode(nextCharacter)));
        return &(m_map.find(nextCharacter)->second);
    }

    return NULL;
}

WordNode *WordTree::insert(string &keyword)
{
    return insert(&m_emptyRoot, keyword);
}

WordNode *WordTree::insert(const char *keyword)
{
    string wordstr(keyword);
    return insert(wordstr);
}

WordNode *WordTree::find(string &keyword)
{
    return find(&m_emptyRoot, keyword);
}

WordNode *WordTree::insert(WordNode *parent, string &keyword)
{
    if (keyword.empty())
    {
        return NULL;
    }

    string firstChar = keyword.substr(0, PACE);
    auto firstNode = parent->findChild(firstChar);
    if (!firstNode)
    {
        return insertBranch(parent, keyword);
    }

    string restChar = keyword.substr(PACE, keyword.size());
    return insert(firstNode, restChar);
}

WordNode *WordTree::insertBranch(WordNode *parent, string &keyword)
{
    string firstChar = keyword.substr(0, PACE);
    auto firstNode = parent->insertChild(firstChar);
    if (firstNode)
    {
        string restChar = keyword.substr(PACE, keyword.size());
        if (!restChar.empty())
        {
            return insertBranch(firstNode, restChar);
        }
    }

    return NULL;
}

WordNode *WordTree::find(WordNode *parent, string &keyword)
{
    string firstChar = keyword.substr(0, PACE);
    auto firstNode = parent->findChild(firstChar);
    if (!firstNode)
    {
        count = 0;
        return NULL;
    }

    string restChar = keyword.substr(PACE, keyword.size());
    if (firstNode->m_map.empty())
    {
        return firstNode;
    }

    if (keyword.size() == PACE)
    {
        return NULL;
    }

    count++;
    return find(firstNode, restChar);
}

WordFilter *WordFilter::sharedInstace()
{
    if (pInstace)
    {
        return pInstace;
    }

    pInstace = new WordFilter;
    return pInstace;
}

void WordFilter::release()
{
    if (pInstace)
    {
        delete pInstace;
    }

    pInstace = NULL;
}

void WordFilter::load(const char *filepath)
{
    wbl::WriteLocker lock(m_rwlock);
    ifstream infile(filepath, ios::in);
    if (!infile)
    {
        ROLLLOG_ERROR << "open file error" << endl;
        return;
    }

    ROLLLOG_DEBUG << "open file succeed" << endl;

    string read;
    while (getline(infile, read))
    {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        string s;
        s = read.substr(0, read.length() - 1);
        m_tree.insert(s);
#endif
    }

    infile.close();
}

void WordFilter::load(const vector<string> &words)
{
    wbl::WriteLocker lock(m_rwlock);
    for (auto it = words.begin(); it != words.end(); ++it)
    {
        string s = *it;
        m_tree.insert(s);
    }
}

bool WordFilter::censor(string &source)
{
    wbl::WriteLocker lock(m_rwlock);

    bool bRet = false;
    int lenght = source.size();
    for (int i = 0; i < lenght; i += 1)
    {
        string substring = source.substr(i, lenght - i);
        if (m_tree.find(substring) != NULL)
        {
            source.replace(i, (m_tree.count + 1), "**");
            lenght = source.size();
            bRet = true;
            // cout << "source = " <<  source << endl;
        }
    }

    return bRet;
}
