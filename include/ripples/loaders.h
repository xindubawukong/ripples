//===------------------------------------------------------------*- C++ -*-===//
//
//             Ripples: A C++ Library for Influence Maximization
//                  Marco Minutoli <marco.minutoli@pnnl.gov>
//                   Pacific Northwest National Laboratory
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2019, Battelle Memorial Institute
//
// Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
// to any person or entity lawfully obtaining a copy of this software and
// associated documentation files (hereinafter “the Software”) to redistribute
// and use the Software in source and binary forms, with or without
// modification.  Such person or entity may use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and may permit
// others to do so, subject to the following conditions:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimers.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Other than as used herein, neither the name Battelle Memorial Institute or
//    Battelle may be used in any form whatsoever without the express written
//    consent of Battelle.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//===----------------------------------------------------------------------===//

#ifndef RIPPLES_LOADERS_H
#define RIPPLES_LOADERS_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "ripples/diffusion_simulation.h"
#include "ripples/graph.h"
#include "trng/lcg64.hpp"
#include "trng/truncated_normal_dist.hpp"
#include "trng/uniform01_dist.hpp"

namespace ripples {

//! Edge List in TSV format tag.
struct edge_list_tsv {};
//! Weighted Edge List in TSV format tag.
struct weighted_edge_list_tsv {};

namespace {

inline uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >> 4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v << 5;
  return v;
}

double Uniform(size_t n, size_t u, size_t v, double l, double r) {
  if (u > v) std::swap(u, v);
  double p = 1.0 * hash64(hash64(hash64(n) + u + 1) + v + 1) / std::numeric_limits<uint64_t>::max();
  return l + (r - l) * p;
}

//! Load an Edge List in TSV format and generate the weights.
//!
//! \tparam EdgeTy The type of edges.
//! \tparam PRNG The type of the parallel random number generator.
//! \tparam diff_model_tag The Type-Tag for the diffusion model.
//!
//! \param inputFile The name of the input file.
//! \param undirected When true, the edge list is from an undirected graph.
//! \param rand The random number generator.
template <typename EdgeTy, typename PRNG, typename diff_model_tag>
std::vector<EdgeTy> load(const std::string &inputFile, const bool undirected,
                         PRNG &rand, const edge_list_tsv &&,
                         const diff_model_tag &&) {
  std::ifstream GFS(inputFile);
  size_t lineNumber = 0;

  trng::uniform01_dist<float> probability;

  std::vector<EdgeTy> result;

  std::cout << "loading an unweighted graph" << std::endl;

  if (inputFile.back() == 'n') {
    std::cout << "load graph from " << inputFile << std::endl;
    std::ifstream ifs(inputFile);
    if (!ifs.is_open()) {
      std::cerr << "Error: Cannot open file " << inputFile << '\n';
      abort();
    }
    size_t n, m, sizes;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
    assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);

    // graph.n = n;
    // graph.m = m;
    std::vector<uint64_t> offset(n + 1);
    std::vector<uint32_t> edge(m);
    ifs.read(reinterpret_cast<char*>(offset.data()), (n + 1) * 8);
    ifs.read(reinterpret_cast<char*>(edge.data()), m * 4);
    std::cout << "n: " << n << std::endl;
    std::cout << "m: " << m << std::endl;
    result.reserve(m);
    for (typename EdgeTy::vertex_type u = 0; u < n; u++) {
      if (u % 1000000 == 0) {
        std::cout << "u: " << u << std::endl;
      }
      for (size_t j = offset[u]; j < offset[u + 1]; j++) {
        typename EdgeTy::vertex_type v = edge[j];
        auto weight = rand();
        EdgeTy e = {u, v, weight};
        result.emplace_back(e);
      }
    }
    // graph.offset = sequence<EdgeId>(n + 1);
    // graph.E = sequence<NodeId>(m);
    // parallel_for(0, n + 1, [&](size_t i) { graph.offset[i] = offset[i]; });
    // parallel_for(0, m, [&](size_t i) { graph.E[i] = edge[i]; });
    if (ifs.peek() != EOF) {
      std::cerr << "Error: Bad data\n";
      abort();
    }
    ifs.close();
    std::cout << "read done " << result.size() << std::endl;
    return result;
  }

  for (std::string line; std::getline(GFS, line); ++lineNumber) {
    if (line.empty()) continue;
    if (line.find('%') != std::string::npos) continue;
    if (line.find('#') != std::string::npos) continue;
    if (lineNumber % 10000000 == 0) {
      std::cout << "lineNumber: " << lineNumber << std::endl;
    }

    std::stringstream SS(line);

    typename EdgeTy::vertex_type source;
    typename EdgeTy::vertex_type destination;
    typename EdgeTy::weight_type weight;
    SS >> source >> destination;

    weight = rand();
    EdgeTy e = {source, destination, weight};
    result.emplace_back(e);

    if (undirected) {
      weight = rand();
      EdgeTy e = {destination, source, weight};
      result.emplace_back(e);
    }
  }

  if (std::is_same<diff_model_tag, ripples::linear_threshold_tag>::value) {
    auto cmp = [](const EdgeTy &a, const EdgeTy &b) -> bool {
      return a.source < b.source;
    };

    std::sort(result.begin(), result.end(), cmp);

    for (auto begin = result.begin(); begin != result.end();) {
      auto end = std::upper_bound(begin, result.end(), *begin, cmp);
      typename EdgeTy::weight_type not_taking = rand();
      typename EdgeTy::weight_type total = std::accumulate(
          begin, end, not_taking,
          [](const typename EdgeTy::weight_type &a, const EdgeTy &b) ->
          typename EdgeTy::weight_type { return a + b.weight; });

      std::transform(begin, end, begin, [=](EdgeTy &e) -> EdgeTy {
        e.weight /= total;
        return e;
      });

      begin = end;
    }
  }

  return result;
}

//! Load a Weighted Edge List in TSV format.
//!
//! \tparam EdgeTy The type of edges.
//! \tparam PRNG The type of the parallel random number generator.
//! \tparam diff_model_tag The Type-Tag for the diffusion model.
//!
//! \param inputFile The name of the input file.
//! \param undirected When true, the edge list is from an undirected graph.
//! \param rand The random number generator.
template <typename EdgeTy, typename PRNG, typename diff_model_tag>
std::vector<EdgeTy> load(const std::string &inputFile, const bool undirected,
                         PRNG &rand, const weighted_edge_list_tsv &&,
                         diff_model_tag &&) {
  std::ifstream GFS(inputFile);
  size_t lineNumber = 0;

  std::vector<EdgeTy> result;

  std::cout << "loading a weighted edge list" << std::endl;

  if (inputFile.back() == 'n') {
    std::cout << "load graph from " << inputFile << std::endl;
    std::cout << "using Uniform weights" << std::endl;
    std::ifstream ifs(inputFile);
    if (!ifs.is_open()) {
      std::cerr << "Error: Cannot open file " << inputFile << '\n';
      abort();
    }
    size_t n, m, sizes;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
    assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);

    // graph.n = n;
    // graph.m = m;
    std::vector<uint64_t> offset(n + 1);
    std::vector<uint32_t> edge(m);
    ifs.read(reinterpret_cast<char*>(offset.data()), (n + 1) * 8);
    ifs.read(reinterpret_cast<char*>(edge.data()), m * 4);
    std::cout << "n: " << n << std::endl;
    std::cout << "m: " << m << std::endl;
    std::vector<size_t> deg(n);
    for (typename EdgeTy::vertex_type u = 0; u < n; u++) {
      deg[u] = offset[u + 1] - offset[u];
    }
    result.reserve(m);
    long double tott = 0;
    long long cntt = 0;
    for (typename EdgeTy::vertex_type u = 0; u < n; u++) {
      if (u % 1000000 == 0) {
        std::cout << "u: " << u << std::endl;
      }
      for (size_t j = offset[u]; j < offset[u + 1]; j++) {
        typename EdgeTy::vertex_type v = edge[j];
        // float weight = Uniform(n, u, v, 0.1, 0.3);
        float weight = 2.0 / (deg[u] + deg[v]);
        EdgeTy e = {u, v, weight};
        tott += weight;
        if (weight > 0.2) cntt++;
        assert(weight >= 0 && weight <= 1);
        result.emplace_back(e);
      }
    }
    std::cout << "cntt: " << cntt << "  avg: " << tott / result.size() << std::endl;
    // graph.offset = sequence<EdgeId>(n + 1);
    // graph.E = sequence<NodeId>(m);
    // parallel_for(0, n + 1, [&](size_t i) { graph.offset[i] = offset[i]; });
    // parallel_for(0, m, [&](size_t i) { graph.E[i] = edge[i]; });
    if (ifs.peek() != EOF) {
      std::cerr << "Error: Bad data\n";
      abort();
    }
    ifs.close();
    std::cout << "read done " << result.size() << std::endl;
    return result;
  }

  for (std::string line; std::getline(GFS, line); ++lineNumber) {
    if (line.empty()) continue;
    if (line.find('%') != std::string::npos) continue;
    if (line.find('#') != std::string::npos) continue;

    std::stringstream SS(line);

    typename EdgeTy::vertex_type source;
    typename EdgeTy::vertex_type destination;
    typename EdgeTy::weight_type weight;
    SS >> source >> destination >> weight;

    EdgeTy e = {source, destination, weight};
    result.emplace_back(e);

    if (undirected) {
      EdgeTy e = {destination, source, weight};
      result.emplace_back(e);
    }
  }
  return result;
}

}  // namespace

template <typename PRNG, typename Distribution>
class WeightGenerator {
 public:
  WeightGenerator(PRNG &gen, Distribution dist, float scale_factor = 1.0)
      : gen_(gen), dist_(dist), scale_factor_(scale_factor) {}

  WeightGenerator(PRNG &gen, float scale_factor = 1.0)
      : WeightGenerator(gen, Distribution(), scale_factor) {}

  float operator()() { return scale_factor_ * dist_(gen_); }

 private:
  PRNG gen_;
  Distribution dist_;
  float scale_factor_;
};

//! Load an Edge List.
//!
//! \tparam EdgeTy The type of edges.
//! \tparam Configuration The type describing the input of the tool.
//! \tparam PRNG The type of the parallel random number generator.
//!
//! \param CFG The input configuration.
//! \param weightGen The random number generator used to generate the weights.
template <typename EdgeTy, typename Configuration, typename PRNG>
std::vector<EdgeTy> loadEdgeList(const Configuration &CFG, PRNG &weightGen) {
  std::vector<EdgeTy> edgeList;
  if (CFG.weighted) {
    if (CFG.diffusionModel == "IC") {
      edgeList = load<EdgeTy>(CFG.IFileName, CFG.undirected, weightGen,
                              ripples::weighted_edge_list_tsv{},
                              ripples::independent_cascade_tag{});
    } else if (CFG.diffusionModel == "LT") {
      edgeList = load<EdgeTy>(CFG.IFileName, CFG.undirected, weightGen,
                              ripples::weighted_edge_list_tsv{},
                              ripples::linear_threshold_tag{});
    }
  } else {
    if (CFG.diffusionModel == "IC") {
      edgeList = load<EdgeTy>(CFG.IFileName, CFG.undirected, weightGen,
                              ripples::edge_list_tsv{},
                              ripples::independent_cascade_tag{});
    } else if (CFG.diffusionModel == "LT") {
      edgeList = load<EdgeTy>(CFG.IFileName, CFG.undirected, weightGen,
                              ripples::edge_list_tsv{},
                              ripples::linear_threshold_tag{});
    }
  }
  return edgeList;
}

namespace {
template <typename GraphTy, typename ConfTy, typename PrngTy>
GraphTy loadGraph_helper(ConfTy &CFG, PrngTy &PRNG) {
  GraphTy G;

  if (!CFG.reload) {
    using vertex_type = typename GraphTy::vertex_type;
    using weight_type = typename GraphTy::edge_type::edge_weight;
    using edge_type = ripples::Edge<vertex_type, weight_type>;
    auto edgeList = ripples::loadEdgeList<edge_type>(CFG, PRNG);
    std::cout << "edgeList done" << std::endl;
    GraphTy tmpG(edgeList.begin(), edgeList.end(), !CFG.disable_renumbering);
    G = std::move(tmpG);
    std::cout << "move done" << std::endl;
  } else {
    std::ifstream binaryDump(CFG.IFileName, std::ios::binary);
    GraphTy tmpG(binaryDump);
    G = std::move(tmpG);
  }

  return G;
}
}  // namespace

//! Load Graphs.
//!
//! \tparam GraphTy The type of the graph to be loaded.
//! \tparam ConfTy  The type of the configuration object.
//! \tparam PrngTy  The type of the parallel random number generator object.
//!
//! \param CFG The configuration object.
//! \param PRNG The parallel random number generator.
//! \return The GraphTy graph loaded from the input file.
template <typename GraphTy, typename ConfTy, typename PrngTy>
GraphTy loadGraph(ConfTy &CFG, PrngTy &PRNG) {
  GraphTy G;
  if (CFG.distribution == "uniform") {
    WeightGenerator<trng::lcg64, trng::uniform01_dist<float>> gen(
        PRNG, CFG.scale_factor);
    G = loadGraph_helper<GraphTy>(CFG, gen);
  } else if (CFG.distribution == "normal") {
    WeightGenerator<trng::lcg64, trng::truncated_normal_dist<float>> gen(
        PRNG,
        trng::truncated_normal_dist<float>(CFG.mean, CFG.variance, 0.0, 1.0),
        CFG.scale_factor);
    G = loadGraph_helper<GraphTy>(CFG, gen);
  } else if (CFG.distribution == "const") {
    auto gen = [&]() -> float { return CFG.mean; };
    G = loadGraph_helper<GraphTy>(CFG, gen);
  } else {
    throw std::domain_error("Unsupported distribution");
  }
  return G;
}

}  // namespace ripples

#endif /* LOADERS_H */
