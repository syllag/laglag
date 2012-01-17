#define ASS_TRUE 1
#define ASS_FALSE 2
#define NOT_ASS 0

#define LIT_TRUE(l, ass) ((ass) && (((l)^(ass))&1))
#define LIT_FALSE(l, ass) ((ass) && (((l)^((ass)>>1))&1))
#define LIT_UNDEF(ass) (!(ass))
#define LIT(l) ((SIGN(l))?-ABS(l):ABS(l))

#define BIGINT long long int

/************************************/
/* Constant parameters              */
/************************************/

#define SAT 1
#define UNS 2
#define OUT 3

/* Constant for heuristic */

#define TRUE 1
#define FALSE 0

#define MAXLENGTH 200000           /* maximum number of literals which can be in any clause */


#define SIGN(p) ((p)&1)
#define ABS(x) (x>>1)
#define NEG(x) ((x) ^ 1)


typedef struct _node
{
  unsigned int valNode;             /* la dernier point de choix ayant conduit à ce noeud */
  int nbNotExploit;                 /* le nombre de noeuds qui faut encore construire */
  unsigned int *valNodeNotExploit;  /* la valeur des noeuds encore à exploiter */
  int nbSon;                        /* number of open son */
  double sumWeight;                 /* la somme des gains */
  int pi;                           /* le nombre de lancé effectué sur cette branche*/
  int indiceClause;                 /* l'indice de la clause courante */
  struct _node *father;             /* le père du noeud courant */
} node;


void termineProg(int res,char *comment);
void afficheNoeud(node *n);
