#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>

#include "laglag.h"

/****************************************/
/* Compilation flags                    */
/****************************************/
#define TIRETS 83


/****************************************/
/* Main data structures                 */
/****************************************/
unsigned int numatom = 0;
unsigned int numclause = 0;
int numliterals;
unsigned int ** clause = NULL;	/* clauses to be satisfied */
				/* indexed as clause[clause_num][literal_num] */
unsigned int *infoAtom;		/* value of each atom */
unsigned int *clauseTmp;        /* tempory clause */
int sizeClauseTmp;              /* size of the tempory clause */

node *saveBestNode;
unsigned int nbTry;

/* No longer using constant CLK_TCK - it is deprecated! */
/* Instead: */
long ticks_per_second;
int resultat = OUT;


double magicConstant = 1 / 4.25;

/****************************************/
/* Main data structures  of the solveur */
/****************************************/
int numtry = 0;			/* total attempts at solutions */
int nbNode = 0;
node **tabNodeOpen = NULL;;

/* Randomization */
unsigned int seed;  /* Sometimes defined as an unsigned long int */

struct timeval tv;
struct timezone tzp;

/* Statistics */
double expertime;
double totalTime=0;

/************************************/
/* Forward declarations             */
/************************************/
void parse_parameters(int argc,char *argv[]);
double elapsed_seconds(void);
void initprob(void); 
void handle_interrupt(int sig);
void print_statistics_header(void);
void showInter();
void showFinal();
void solve();

/***************************************************************************************/
/* Memory used     *********************************************************************/
/***************************************************************************************/

int memReadStat(int field)
{
  char    name[256];
  pid_t pid = getpid();
  int     value = 0;
  sprintf(name, "/proc/%d/statm", pid);
  FILE*   in = fopen(name, "rb");
  if (in == NULL) return 0;
  for (; field >= 0; field--)
    assert(fscanf(in, "%d", &value));
  fclose(in);
  return value;
}

uint64_t memUsed() 
{
  return (uint64_t)memReadStat(0) * (uint64_t)getpagesize();
}

/*****************************************************************************************/
/* Main           ************************************************************************/
/*****************************************************************************************/
int main(int argc,char *argv[])
{
  ticks_per_second = sysconf(_SC_CLK_TCK);
  gettimeofday(&tv,&tzp);
  seed = 0;//(unsigned int)((( tv.tv_sec & 0177 ) * 1000000) + tv.tv_usec);
  parse_parameters(argc, argv);
  srandom(seed);
  initprob();
  
  print_statistics_header();

  signal(SIGINT, handle_interrupt);
  signal(SIGTERM, handle_interrupt);
  signal(SIGALRM, handle_interrupt);

  (void) elapsed_seconds();
  solve();  
  termineProg(OUT,"UNKNOWN");
  return 0;
}/* main */


/**
   Parse parameters
 */
void parse_parameters(int argc,char *argv[])
{
}/* parse_parameters */


/**
   Handle interrupt
*/
void handle_interrupt(int sig)
{
  showFinal();
  exit(0);
}/* handle_interrupt */


/**
   Init prob : parse problem
*/
void initprob(void)
{
  int i;
  int j = 0;
  int lastc;
  int nextc;
  int storeptr[MAXLENGTH];
  int lit;
  int tau;
  int sz;

  while ((lastc = getchar()) == 'c' || lastc == '\n')
    {
      while ((nextc = getchar()) != EOF && nextc != '\n');
    }
  ungetc(lastc,stdin);
  if (scanf("p cnf %i %i",&numatom, &numclause) != 2)
    {
      fprintf(stderr,"c initprob 1 : Bad input file\n");
      exit(-1);
    }
  while (getchar()!='\n');
  infoAtom = (unsigned int *) malloc((numatom + 1) * sizeof(unsigned int));


  numliterals = 0;
  for(i = 0 ; i < numclause ; i++)
    {
      sz = -1;
      tau = 0;
      do
        {
          sz++;
	  j = scanf("%d", &lit);          
          
          if(j != 1)
	    {
	      lit = 0;
	      
	      if(j == EOF)
		{
		  numclause = i + 1;
		  if(!(sz))
		    {
		      numclause--;
                      return;
		    }
		}
	    }
          
          /* show lit exist to clause[i] or it's a tauto */
	  if(lit)
	    {
	      for(j=0 ; j<sz ; j++) if((storeptr[j] == lit) || (storeptr[j] == -lit)) break;
	      if(j < sz)
		{
		  if(storeptr[j] == lit)
		    {
		      sz--;
		      continue;
		    }/*redundancy*/
		  else tau = 1;/*tautologie*/
		}
	    }
          
          if(lit != 0)
            {
              storeptr[sz] = lit; /* clause[i][size[i]] = j; */
              numliterals++;
            }
        }
      while(lit != 0);
      
      if(!sz)
        {
          printf("s UNSATISFIABLE\n");
          exit(20);
        }
      
      if(tau)
        {
          numclause--;
          i--;
        }
      else
        {
          if(!(clause = (unsigned int **) realloc(clause, sizeof(unsigned int *) * (i + 1))))
            {perror("clause"); exit(1);}
          if(!(clause[i] = (unsigned int *) malloc(sizeof(unsigned int)*(sz + 2))))
            {
              perror("size");
              exit(1);
            }	  

          for(j = 1 ; j <= sz ; j++)
            {
              if(storeptr[j -1] > 0)
                clause[i][j] = storeptr[j - 1]<<1;
              else clause[i][j] = ((-storeptr[j - 1])<<1) + 1;
            }
          *clause[i] = sz;
        }
    }  
}/* initprob */


/**
   elapsed seconds
 */
double elapsed_seconds(void) 
{ 
   double answer;   
   answer = (double) clock() / CLOCKS_PER_SEC - totalTime;
   totalTime = (double) clock() / CLOCKS_PER_SEC;
   return answer; 
}/* elapsed_seconds */



/**
   Print statistics header
 */
void print_statistics_header(void)
{
  int i;
  fprintf(stderr, "c \nc seed = %i, numatom = %i, numclause = %i, numliterals = %i\nc ",
          seed, numatom,numclause,numliterals);

  for(i = 0 ; i< TIRETS ; fprintf(stderr, "-"), i++); fprintf(stderr, "\n");
}/* print_statistics_header */


/**
   Fonction permettant d'afficher les informations intermédiaire
 */
void showInter()
{

}/* showInter */

/**
   Fonction permettant de savoir le temps passé depuis le début du programme
   Retourne :
      - le temps passé depuis le début du programme
 */
double tps_ecouler()
{
   double answer;
   answer = (double) clock() / CLOCKS_PER_SEC;
   return answer; 
}//tps_ecouler


/**
   Fonction permettant d'afficher les r<E9>sultats du test
 */
void showFinal()
{
  int i;


  showInter();
  fprintf(stderr, "c ");
  for(i = 0 ; i< TIRETS ; fprintf(stderr, "="), i++); fprintf(stderr, "\n");
  fprintf(stderr, "c times = %lf\n", tps_ecouler());  

  uint64_t mem_used = memUsed();
  if(mem_used != 0) fprintf(stderr,"c Memory used           : %.2f MB\n", mem_used / 1048576.0);

  switch(resultat)
    {
    case SAT :
      {
        printf("s SATISFIABLE\n");
        printf("v ");
        for(i = 1 ; i<=numatom ; i++)
          if(infoAtom[i] == ASS_TRUE) printf("%d ", i); else printf("%d ", -i);
        printf("0\n");
        exit(10);
        break;
      }
    case UNS :
      {
        printf("s UNSATISFIABLE\n");
        exit(20);
        break;
      }
    case OUT : 
      {
        printf("UNKNOWN\n");
      }
    }  
}//showFinal


/**
   Fonction permettant de gérer la terminaison du programme
   En entrée : 
      - res, dans quel état le programme c'est terminé
      - comment, une chaine de caractère décrivant l'état de sorti
 */
void termineProg(int res,char *comment)
{
  resultat = res;
  kill(getpid(),SIGTERM);
}/* termineProg */

/*                                                                             */
/*            @      @@@@@  @@@@@       @      @@@@@  @@@@@                    */
/*            @      @   @  @           @      @   @  @                        */
/*            @      @@@@@  @  @@  @@@  @      @@@@@  @  @@                    */
/*            @      @   @  @   @       @      @   @  @   @                    */
/*            @@@@@  @   @  @@@@        @@@@@  @   @  @@@@                     */
/*                                                                             */


/**
   Init solver
 */
void initSolver()
{
  int i;

  nbTry = 0;
  clauseTmp = (unsigned int  *) calloc(numatom, sizeof(unsigned int));
  sizeClauseTmp = 0;

  tabNodeOpen = (node **) calloc(1, sizeof(node *));
  tabNodeOpen[0] = (node *) calloc(1, sizeof(node));
  tabNodeOpen[0]->nbNotExploit = **clause;
  tabNodeOpen[0]->valNodeNotExploit = (unsigned int *) malloc(**clause*sizeof(unsigned int));
  for(i = 1 ; i<=**clause; i++) tabNodeOpen[0]->valNodeNotExploit[i - 1] = clause[0][i];
  tabNodeOpen[0]->indiceClause = 0;
  saveBestNode = tabNodeOpen[0];
  nbNode++;
}

/**
   Fonction de comparaison de noeuds très simple.
 */
int cmpSimple(node *n1, node *n2)
{
  return n1->sumWeight - n2->sumWeight;
}/* cmpSimple */

/**
   Select a node.
   retourne un noeud dans le tableau tabNodeOpen.
 */
node *selectNode()
{
  return saveBestNode;
}/* selectNode */

/**
   Fonction permettant d'afficher une clause
 */
void afficheClause(unsigned int *cl)
{
  int i;
  printf("============> ");
  for(i = 1 ; i <= *cl ; i++) printf("%d ", LIT(cl[i])); 
  printf("\n");
}/* afficheClause */


/**
  Open a node.
  Retourne le noeud fraichement ouvert.
 */
node *openNode(node *n)
{
  int i, j;
  unsigned int *tmpCl;
  node *p;
  unsigned int varSelect;

  /* recup the partial interpretation */
  memset(infoAtom, NOT_ASS, (numatom + 1) * sizeof(unsigned int));
  for(p = n ; p ; infoAtom[ABS(p->valNode)] = SIGN(p->valNode) ? ASS_FALSE : ASS_TRUE, p = p->father);
  
 searchOpenNode:
  if(!n->nbNotExploit) return NULL;

  /* select a new variable */
  j = random() % n->nbNotExploit;
  varSelect = n->valNodeNotExploit[j];
  n->valNodeNotExploit[j] = n->valNodeNotExploit[--n->nbNotExploit];
  infoAtom[ABS(varSelect)] = (varSelect & 1) ? ASS_FALSE : ASS_TRUE;
  
  /* search the next clause */
  for(i = n->indiceClause + 1 ; i<numclause ; i++)
    {
      sizeClauseTmp = 0;
      tmpCl = clause[i];
      
      for(j = 1 ; j <= *tmpCl ; j++)
        {
          if(infoAtom[ABS(tmpCl[j])] == NOT_ASS)
            clauseTmp[sizeClauseTmp++] = tmpCl[j];          
          else if(LIT_TRUE(tmpCl[j], infoAtom[ABS(tmpCl[j])])) break; /* this clause is SAT */
        }      
      if(j<=*tmpCl) continue;
      break; /* j'ai ma clause (i) et les littéraux libres de celle-ci (clauseTmp) */
    } 

  if(i >= numclause) termineProg(SAT, "SAT"); /* SAT is found */

  if(!sizeClauseTmp) /* this node is closed (unsat) */
    {
      infoAtom[ABS(varSelect)] = NOT_ASS;    
      goto searchOpenNode;
    }
  
  /* create the new node */
  tabNodeOpen = (node **) realloc(tabNodeOpen, sizeof(node *) * (nbNode + 1));
  p = tabNodeOpen[nbNode] = (node *) calloc(1, sizeof(node));
  p->indiceClause = i;
  p->nbNotExploit = sizeClauseTmp;
  p->father = n;
  p->valNode = varSelect;
  p->valNodeNotExploit = (unsigned int *) malloc(sizeClauseTmp * sizeof(unsigned int));
  for(j = 0 ; j < sizeClauseTmp ; j++) p->valNodeNotExploit[j] = clauseTmp[j];

  n->nbSon++;
  nbNode++;  
  return p;
}/* openNode */


/**
   Simulation
 */
int simulation(node *n)
{
  int i, j;
  int sizeClauseTmp;
  unsigned int *cl, lit;

  for(i = n->indiceClause ; i<numclause ; i++)
    {
      sizeClauseTmp = 0;
      cl = clause[i];

      for(j = 1 ;  j <= *cl ; j++)
        {
          lit = cl[j];
          if(LIT_FALSE(lit, infoAtom[ABS(lit)])) continue;
          if(LIT_TRUE(lit, infoAtom[ABS(lit)])) break;
          clauseTmp[sizeClauseTmp++] = lit;
        }

      if(j <= *cl) continue;
      if(!sizeClauseTmp) return i;
      lit = clauseTmp[random() % sizeClauseTmp];
      infoAtom[ABS(lit)] = (SIGN(lit)) + 1; /* (SIGN(lit) ? ASS_FALSE : ASS_TRUE */
    }

  if(i == numclause) termineProg(SAT, "SAT");
  return i;
}/* simulation */ 

/**
   Delete node
 */
void deleteNode(node *nd)
{
  if(nd->nbSon) return; /* there is still nodes */ 

  while(nd && !nd->nbSon && !nd->nbNotExploit)
    { 
      nd = nd->father;
      if(nd) nd->nbSon--;
    }
}/* deleteNode */


/**
   Calcul du poids. Attention save truc compliqué doit être calculé.
 */
double computeWeight(node *n)
{
  //return (n->sumWeight / nbTry) + magicConstant * sqrt(log(nbTry) / n->pi);
  return (n->sumWeight / n->pi) + magicConstant * sqrt(log(nbTry) / n->pi);
}/* computeWeight */

/**
   Parcours l'ensemble des noeuds afin d'ajuster les poids des
   clauses. Sauvegarde aussi dans saveBestNode le noeud qui possède le
   meilleur score.
 */
void adjustWeights(double (*computeW)(node *))
{
  int i = 0;
  node *nodeTmp;
  saveBestNode = NULL;
  double bestScore = 0, currentScore = 0;

  while(i<nbNode)
    {
      nodeTmp = tabNodeOpen[i];

      if(!nodeTmp->nbNotExploit)
	{
	  if(!nodeTmp->nbSon) tabNodeOpen[i] = tabNodeOpen[--nbNode]; else i++;
	  continue;
	}
      
      currentScore = computeW(nodeTmp);
      if(!saveBestNode || (bestScore < currentScore))
        {
          saveBestNode = nodeTmp;
          bestScore = currentScore;
        }
      i++;
    }

  if(!saveBestNode) termineProg(UNS, "UNSAT");
  assert(saveBestNode);
}/* adjustWeight */

/**
   Fonction permettant d'afficher un noeud
 */
void afficheNoeud(node *n)
{
  int i;

  printf("--- n = %p val = %d nbSon = %d indCl = %d weight = %.2lf pi = %d father = %p ---\n",
         n, LIT(n->valNode), n->nbSon, n->indiceClause, 
	 n->sumWeight, n->pi, n->father);
  for(i = 0 ; i<n->nbNotExploit ; i++)
    printf("%d ", LIT((n->valNodeNotExploit)[i]));
  printf("\n------------------------------------------\n\n");
}/* afficheNoeud */


/**
   Fonction permettant d'afficher l'ensemble des noeuds ouvert.
 */
void affiche()
{
  int i;
  node *tmp;

  printf("====== On affiche tous les noeuds =====\n");
  for(i = 0 ; i<nbNode ; i++)
    {
      tmp = tabNodeOpen[i];
      afficheNoeud(tmp);
    }
}/* affiche */


/**
   laglag solver
 */
void solve()
{
  unsigned int score;
  node *hasOpen = NULL, *newNode = NULL;
  
  initSolver();

  while(nbNode)
    {
      if(!(nbTry & 16383)) printf("nbTry = %d \t nbNode = %d\n", nbTry, nbNode);
      nbTry++;
      hasOpen = selectNode();
      newNode = openNode(hasOpen);
    
      if(!newNode) /* the node pointed by indice is closed */        
          deleteNode(hasOpen);
      else
	{
	  score = simulation(newNode);
	  
	  /* propage the score */
	  for( ; newNode ; newNode = newNode->father)
	    {
	      newNode->pi++;
	      newNode->sumWeight += score;
	    }
	}

      adjustWeights(computeWeight);
    }
}//solve
