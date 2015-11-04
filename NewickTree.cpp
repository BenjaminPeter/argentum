#include "NewickTree.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
using namespace std;

/**
 * Update the cluster distance values of 'src_leaf' by traversing whole tree
 */
void NewickTree::distanceByTraversal(Node *pn, Node *src, Node *src_leaf, unsigned maxval, vector<unsigned> &cd)
{
    maxval = max(maxval, pn->size);
    if (pn->leaf)
    {
        unsigned i = pn->lid;
        unsigned j = src_leaf->lid;
        assert (i < root_->size && j < root_->size);
        cd[i + root_->size * j] = maxval;
        return;
    }
    
    for (set<Node *>::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
        if (*it != src) // Assert: src has already been processed
            distanceByTraversal(*it, pn, src_leaf, maxval, cd);
    if (pn->parent != 0 && pn->parent != src)
        distanceByTraversal(pn->parent, pn, src_leaf, maxval, cd);
}
/**
 * Update cluster distance values over all leaf node pairs
 */
void NewickTree::subtreeDistance(vector<unsigned> &cd)
{
    assert (cd.size() == root_->size*root_->size);
    for (unsigned i = 0; i < cd.size(); ++i)
        cd[i] = 0;
    for (unsigned i = 0; i < leaves.size(); ++i)
        distanceByTraversal(leaves[i]->parent, leaves[i], leaves[i], 1, cd);
}

// Create a new tree from the given file
NewickTree::NewickTree(string const &filename)
    : root_(0), leaves(), valid(0), fp(0), good_(false) 
{
    // Open file handle, "-" uses standard input
    if (filename == "-")
        fp = &std::cin;
    else
        fp = new ifstream(filename.c_str());

    if (fp == 0 || !fp->good())
        return;

    string row;
    if (!std::getline(*fp, row).good())
        return;
    while (fp->good() && (row.empty() || row[0] != '[')) // Find first tree
        std::getline(*fp, row);
    if (!fp->good())
        return;
    
    parse(row);
    updateSizes(root_);
    leaves.resize(root_->size, 0);
    collectLeaves(root_);
    assert (root_->size == leaves.size());
    good_ = true;
}

bool NewickTree::next()
{
    if (!good_ || fp == 0 || !fp->good())
        return false;

    // Free previous tree structure
    delete root_;
    root_ = 0;
    leaves.clear();
    good_ = false;
    
    string row;
    if (!std::getline(*fp, row).good())
        return false;
    while (fp->good() && (row.empty() || row[0] != '[')) // Find next tree
        std::getline(*fp, row);
    if (!fp->good())
        return false;
    
    parse(row);
    updateSizes(root_);
    leaves.resize(root_->size, 0);
    collectLeaves(root_);
    assert (root_->size == leaves.size());
    good_ = true;
    return true;
}


NewickTree::~NewickTree()
{
    if (fp != &std::cin) delete fp; fp = 0;
    delete root_;
    root_ = 0;
}


void assignLabels(NewickTree::Node *pn, InputColumn const &ic)
{
    if (pn->leaf)
    {
        // Sets the new column leaf label
        pn->llabel = ic[pn->lid];
        return;
    }
    
    for (set<NewickTree::Node *>::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
        assignLabels(*it, ic);
}

void NewickTree::assignLabels(InputColumn const &ic)
{
    ::assignLabels(root_, ic);
}   

void NewickTree::collectLeaves(Node *pn)
{
    if (pn->leaf)
    {
        leaves[pn->lid] = pn;
        return;
    }
    for (set<Node *>::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
        collectLeaves(*it);
}

// Bottom-up cascade of subtree size (n:o leaves)
int NewickTree::updateSizes(Node *pn)
{
    if (pn->leaf)
    {
        pn->size = 1;
        return 1;
    }
    pn->size = 0;
    for (set<Node *>::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
        pn->size += updateSizes(*it);
    return pn->size;
}

/* depth version */
void NewickTree::updateMaxDists()
{
    updateMaxDists(root_);
}
unsigned NewickTree::updateMaxDists(Node * pn)
{
    if (pn->leaf)
        return 1;

    unsigned maxd = 0;
    for (NewickTree::children_set_t::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
    {
        unsigned d = updateMaxDists(*it);
        if (maxd < d)
            maxd = d;
    }

    pn->mdepth = maxd;
    return maxd+1;
}

int atoi_min_(char const *value, int min)
{
    std::istringstream iss(value);
    int i;
    char c;
    if (!(iss >> i) || iss.get(c))
    {
        cerr << "error: value " << value << " must be of type <int>, and greater than or equal to " << min << endl;
        std::exit(1);
    }

    if (i < min)
    {
        cerr << "error: value " << value << " must be greater than or equal to " << min << endl;
        std::exit(1);
    }
    return i;
}

// Simple tree parser
size_t NewickTree::parse(string const &s, size_t i, Node *p)
{
    while (i != string::npos && i < s.size()-1)
    {
        if (s[i]=='(')
        {
            Node *j = 0;
            if (p == 0)
            {
                j = new Node();
                root_ = j;
            }
            else
                j = new Node(p);
            i = parse(s, i+1, j);
            continue;
        }
        else if (s[i] == ',')
        {
            i+=1;
            continue;
        }
        else if (s[i] == ')')
        {
            i = s.find_first_of(",)(;",i+1);
            return i;
        }
        else
        {
            Node *j = new Node(p);
            j->lid = ::atoi_min_(s.substr(i, s.find_first_of(":",i)-i).c_str(), 1)-1;
            j->llabel = -1;
            j->leaf = true;
            i = s.find_first_of("),",i);
        }
    }
    return i;
}
void NewickTree::parse(string const &s)
{
    if (s[0] != '[')
        cerr << "error on row: " << s << endl;
    assert(s[0] == '[');
    size_t i = s.find_first_of("]");
    valid = ::atoi_min_(s.substr(1,i-1).c_str(), 0);
    i +=1;
    i = parse(s, i, 0);
    assert(s[i] == ';');
}

unsigned NewickTree::outputDOT(Node *pn, unsigned id)
{
    unsigned oid = id;
    if (pn->leaf)
        return id;
    for (set<Node *>::iterator it = pn->ch.begin(); it != pn->ch.end(); ++it)
    {
        id ++;
        cout << " n" << oid << " -> n" << id << endl;
        cout << " n" << id << " [label=\"";
        if ((*it)->lid)
            cout << (*it)->lid;
        else
            cout << id;
        cout << "\"]" << endl;
        id = outputDOT(*it, id);
    }
    return id;
}
/** 
 * Debuging output (Graphwiz DOT format)
 */
void NewickTree::outputDOT()
{
    cout << "digraph G {" << endl;
    outputDOT(root_, 100);
    cout << "}" << endl;
}

