//=============================================================================
// Parsimony algorithm


#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "spidir.h"
#include "common.h"
#include "Tree.h"


#define matind(m, i, j) ((m)*(i) + (j))
#define MAX_COST 1000000000




float subcost[4][4] = {
    {0, 1, 1, 1},
    {1, 0, 1, 1},
    {1, 1, 0, 1},
    {1, 1, 1, 0}
};


struct ParsimonyCell
{
    ParsimonyCell() :
        gap(false)
    {}
    float cost;
    float leftcost;
    float rightcost;
    int leftbase;
    int rightbase;
    bool gap;
};


// assume binary tree
void parsimony_helper(Tree *tree, int nseqs, char **seqs, 
                    ParsimonyCell *table)
{
    for (int i=nseqs; i<tree->nnodes; i++) {
        int left = tree->nodes[i].children[0]->name;
        int right = tree->nodes[i].children[1]->name;
        
        // process this node
        for (int a=0; a<4; a++) {
            int minleft = 0, minright = 0;
            float minleftcost = MAX_COST, minrightcost = MAX_COST;
            float leftsub = 0;
            float rightsub = 0;
            float leftmatch = 2;
            float rightmatch = 2;
            
            for (int b=0; b<4; b++) {
                float sub = subcost[a][b];
                float leftcost = table[matind(4, left, b)].cost + sub;
                float rightcost = table[matind(4, right, b)].cost + sub;
                
                if (leftcost < minleftcost ||
                    (leftcost == minleftcost &&
                     (rand() / float(RAND_MAX)) < (1/leftmatch)))
                {
                    minleftcost = leftcost;
                    minleft = b;
                    leftsub = sub;
                    if (leftcost == minleftcost)
                        leftmatch += 1;                    
                    else
                        leftmatch = 2;
                }
                
                
                if (rightcost < minrightcost ||
                    (rightcost == minrightcost && 
                           (rand() / float(RAND_MAX)) < (1/rightmatch)))
                {
                    minrightcost = rightcost;
                    minright = b;
                    rightsub = sub;
                    
                    if (rightcost == minrightcost)
                        rightmatch += 1;
                    else
                        rightmatch = 2;
                }
            }
            
            // save cost and pointers
            int k = matind(4, i, a);
            table[k].cost = minleftcost + minrightcost;
            table[k].leftcost = leftsub;
            table[k].rightcost = rightsub;
            table[k].leftbase = minleft;
            table[k].rightbase = minright;
            table[k].gap = table[matind(4, left, 0)].gap && \
                           table[matind(4, right, 0)].gap;
        }
    }
}


void getParsimonyCost(Tree *tree, Node *node, int base, ParsimonyCell *table)
{
    if (node->nchildren > 0) {
        Node *left = node->children[0];
        Node *right = node->children[1];
        
        left->dist += table[matind(4, node->name, base)].leftcost;
        right->dist += table[matind(4, node->name, base)].rightcost;
        
        // recurse
        getParsimonyCost(tree, left, table[matind(4, node->name, base)].leftbase, 
                         table);
        getParsimonyCost(tree, right, table[matind(4, node->name, base)].rightbase, 
                         table);
    }
}


// ensures that all characters in the alignment are sensible
// TODO: do not change alignment (keep Ns)
bool checkSequences(int nseqs, int seqlen, char **seqs)
{
    // check seqs
    // CHANGE N's to gaps
    for (int i=0; i<nseqs; i++) {
        for (int j=0; j<seqlen; j++) {
            if (seqs[i][j] == 'N' || seqs[i][j] == 'n')
                // treat Ns as gaps
                seqs[i][j] = '-';
            if (seqs[i][j] != '-' &&
                dna2int[(int) (unsigned char) seqs[i][j]] == -1)
            {
                // a unknown character is in the alignment
                return false;
            }
        }
    }
    
    return true;
}


void parsimony(Tree *tree, int nseqs, char **seqs,
               bool buildAncestral, char **ancetralSeqs)
{
    int seqlen = strlen(seqs[0]);
    
    // allocate dynamic table
    ParsimonyCell *table = new ParsimonyCell [tree->nnodes * 4];
    int *gapless = new int [tree->nnodes];
    
    // initalize distances
    for (int i=0; i<tree->nnodes; i++) {
        tree->nodes[i].dist = 0.0;
        gapless[i] = 0;
    }
    
    
    for (int i=0; i<seqlen; i++) {
        
        // initialize leaves
        // iterate just over the leaves
        
        for (int j=0; j<nseqs; j++) {
            if (seqs[j][i] == '-') {
                // gap
                for (int k=0; k<4; k++) {             
                    table[matind(4, j, k)].cost = 0;
                    table[matind(4, j, k)].gap = true;
                }
            } else {
                for (int k=0; k<4; k++) {
                    table[matind(4, j, k)].cost = MAX_COST;
                    table[matind(4, j, k)].gap = false;
                }
                table[matind(4, j, dna2int[(int) (unsigned char) seqs[j][i]])].cost = 0;
            }
        }
        
        // populate cost table
        parsimony_helper(tree, nseqs, seqs, table);
        
        
        // find min cost at root
        float mincost = MAX_COST;
        int minbase= 0;
        int root = tree->root->name;
        
        for (int a=0; a<4; a++) {
            if (table[matind(4, root, a)].cost < mincost) {
                mincost = table[matind(4, root, a)].cost;
                minbase = a;
            }
        }
        
        // add up dist
        getParsimonyCost(tree, tree->root, minbase, table);
        
        // add up ungapped chars
        for (int j=0; j<tree->nnodes; j++) {
            gapless[j] += table[matind(4, j, 0)].gap ? 0 : 1;
        }
    }
    
    // divide subsitutions by number of sites
    for (int i=0; i<tree->nnodes; i++)
        tree->nodes[i].dist /= gapless[i];
        //float(seqlen);
    // TODO: need to protect against divide by zero
    
    // place root in middle of top branch
    Node *rootnode = tree->root;
    float totlen = rootnode->children[0]->dist + 
                   rootnode->children[1]->dist;
    rootnode->children[0]->dist = totlen / 2.0;
    rootnode->children[1]->dist = totlen / 2.0;
    
    // cleanup
    delete [] table;
    delete [] gapless;
    
}


void parsimony(int nnodes, int *ptree, int nseqs, char **seqs, float *dists,
               bool buildAncestral, char **ancetralSeqs)
{
    int seqlen = strlen(seqs[0]);
    
    // check seqs
    assert(checkSequences(nseqs, seqlen, seqs));
    
    // create tree objects
    Tree tree(nnodes);
    ptree2tree(nnodes, ptree, &tree);
    
    parsimony(&tree, nseqs, seqs, buildAncestral, ancetralSeqs);
    tree.getDists(dists);
}