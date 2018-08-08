#include "MCTS.hpp"
const int kINF = 0x3f3f3f3f;
double MCTS::Cp = 2.0 / sqrt(2);
bool MCTS::shutdown = false;
Graph::Graph(int n) : nvertex(n), graph(n + 1) {}
Graph::Graph(int n, vector<vector<int>> graph_) : nvertex(n), graph(graph_) {}
void Graph::AddEdge(int u, int v) {
  graph[u].push_back(v);
  graph[v].push_back(u);
}

bool Graph::BelongToClique(int vertex, set<int> &clique) {
  if (vertex > nvertex) {
    return true;
  }
  int elements_inside_clique = 0;
  for (int v : graph[vertex]) {
    if (clique.count(v))
      elements_inside_clique++;
  }
  assert(elements_inside_clique <= (int)clique.size());
  return elements_inside_clique == (int)clique.size();
}
/*-----------------------*/
State::State(set<int> clique_, vector<State *> son_, int next_graph_vertex_,
             int nvisited_, double sum_reward_, bool is_terminal_)
    : clique(clique_), son(son_), nvisited(nvisited_),
      next_graph_vertex(next_graph_vertex_), sum_reward(sum_reward_),
      is_terminal(is_terminal_) {}

double State::GetReward(int nvis, double normalize) {
  if (nvisited == 0)
    return kINF;
  return sum_reward / (nvisited * normalize) +
         MCTS::Cp * sqrt((2.0 * log(nvis)) / (double)nvisited);
}
void State::SetNextGraphVertex(Graph *graph, set<int> &clique,
                               vector<int> &perm) {
  while (next_graph_vertex < perm.size() and
         !graph->BelongToClique(perm[next_graph_vertex], clique)) {
    next_graph_vertex++;
  }
}

int State::GetBestChild() {
  assert(nvisited > 0);
  double maxi = -kINF;
  vector<int> best;
  double maxi_mean = -kINF;
  vector<int> idx_terminals;
  for (int i = 0; i < (int)son.size(); i++) {
    if (son[i] != nullptr) {
      if (son[i]->is_terminal) {
        idx_terminals.push_back(i);
      } else {
        maxi_mean = max(maxi_mean, sum_reward / nvisited);
      }
    }
  }
  if (idx_terminals.size() > 0) {
    int tam = idx_terminals.size();
    int vai = rand() % tam;
    return idx_terminals[vai];
  }
  for (int i = 0; i < (int)son.size(); i++) {
    if (son[i] != nullptr) {
      double uct = son[i]->GetReward(nvisited, maxi_mean);
      if (uct > maxi and fabs(uct - maxi) > 1e-8) {
        best.clear();
        maxi = uct;
        best.push_back(i);
      } else if (fabs(uct - maxi) <= 1e-8) {
        best.push_back(i);
      }
    }
  }
  if (best.size() == 0)
    return -1;
  int tam = best.size();
  return best[rand() % tam];
}

/*--------------------------*/

MCTS::MCTS(int n, vector<vector<int>> graph_) {
  graph = new Graph(n, graph_);
  root = new State({}, {}, 0, 0, 0, true);
  shutdown = false;
  for (int i = 1; i <= n; i++)
    perm.push_back(i);
  random_shuffle(perm.begin(), perm.end());
}

void MCTS::SetShutDown(int signum) { shutdown = true; }

set<int> MCTS::Process() {
  int cnt = 0;
  while (root != nullptr and !shutdown) {
    root = Build(root).first;
    if (cnt % 1000000 == 0)
      std::cout << maximum_clique.size() << endl;
    cnt++;
  }
  return maximum_clique;
}

vector<State *> MCTS::GenChildren(State *&tree_vertex) {
  vector<State *> result;

  set<int> clique = tree_vertex->clique;
  int next_graph_vertex = tree_vertex->next_graph_vertex;
  State *with_vertex = nullptr;
  State *without_vertex = nullptr;
  if (next_graph_vertex < (int)perm.size()) {
    without_vertex = new State(clique, {}, next_graph_vertex + 1, 0, 0, true);
    without_vertex->SetNextGraphVertex(graph, clique, perm);
    clique.insert(perm[next_graph_vertex]);
    with_vertex = new State(clique, {}, next_graph_vertex + 1, 0, 0, true);
    with_vertex->SetNextGraphVertex(graph, clique, perm);
    result.push_back(without_vertex);
    result.push_back(with_vertex);
  }
  return result;
}

State *MCTS::Expand(State *&tree_vertex) {
  set<int> clique = tree_vertex->clique;
  vector<State *> son = GenChildren(tree_vertex);
  int next_graph_vertex = tree_vertex->next_graph_vertex;
  int nvisited = 1;
  bool is_terminal = false;
  double sum_reward = Simulation(tree_vertex);
  if (maximum_clique.size() < clique.size())
    maximum_clique = clique;

  if (son.size() > 0) {

    return tree_vertex = new State(clique, son, next_graph_vertex, nvisited,
                                   sum_reward, is_terminal);
  }
  delete tree_vertex;
  tree_vertex = nullptr;
  return nullptr;
}

pair<State *, double> MCTS::Build(State *&tree_vertex) {
  // std::cout << tree_vertex->next_graph_vertex << endl;
  tree_vertex->nvisited++;
  if (tree_vertex->is_terminal) {
    tree_vertex = Expand(tree_vertex);
    if (tree_vertex == nullptr) {
      return {nullptr, -1};
    }
    return {tree_vertex, tree_vertex->sum_reward};
  }
  int idx = tree_vertex->GetBestChild();
  if (idx == -1) {
    tree_vertex->son.clear();
    delete tree_vertex;
    tree_vertex = nullptr;
    return {nullptr, -1};
  }
  pair<State *, double> new_child = Build(tree_vertex->son[idx]);
  tree_vertex->son[idx] = new_child.first;
  tree_vertex->sum_reward += new_child.second;
  return {tree_vertex, new_child.second};
}

double MCTS::Simulation(State *tree_vertex) {
  set<int> clique = tree_vertex->clique;
  int nxt = tree_vertex->next_graph_vertex;
  vector<int> falta;
  for (int i = nxt; i < perm.size(); i++)
    falta.push_back(perm[i]);
  random_shuffle(falta.begin(), falta.end());
  for (int graph_vertex : falta) {
    if (graph->BelongToClique(graph_vertex, clique)) {
      clique.insert(graph_vertex);
    }
  }
  return (double)clique.size();
}
