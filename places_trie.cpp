/*****************************************************************************
 * Yevgeni Kamenski
 * CPSC 5520 Distributed Systems, Seattle University, 2017
 * Project #2: A Three-Tiered Airport Lookup System
 *****************************************************************************/

#include "places_trie.h"
#include <cctype>
#include <algorithm>
#include <fstream>

// Forward declarations for helper functions in this module.
/******************************************************************************/

// Trims whitespace at end
inline std::string& trimRight(std::string& s);

// Removes and erases last word in string. Returns the word.
inline std::string removeLastWord(std::string& s);

// Constructs a city record from places file line. Throws on err.
CityRecord cityRecordFromLine(const std::string &line);

// Loads place records from a file. Throws on IO / parsing / mem errors.
TPlaceRecs loadPlacesFromFile(const char *fName, const size_t approxCount);


// Implementations of public interface methods to init and search
/******************************************************************************/

// State shared between two public api methods
static std::unique_ptr<PlacesTrie> trie;

void initTrie(const char *placesPath, const int recordCountHint) {
  log_printf("Loading from file: %s.", placesPath);
  auto places = loadPlacesFromFile(placesPath, recordCountHint);
  trie = std::make_unique<PlacesTrie>(std::move(places));
  log_printf("Loaded %d places.", trie->size());
}

TrieQueryResult queryPlace(const std::string &city, const std::string &state) {
  // Get set of cities with same name or ambiguous result
  auto result = trie->query(city);

  // Done when found an exact match, or city name is ambiguous
  if (result.places.size() == 1 || result.isAmbiguous) 
    return result;
    
  // When user didn't give us state, but same city is in diff states -> ambig
  if (state.empty()) {
    result.isAmbiguous = true;
    return result;
  }

  // Filter the results more, looking for exact st match (erase-remove idom)
  TFoundPlaces &pl = result.places;
  pl.erase(std::remove_if(pl.begin(), pl.end(), [=](const auto &e) {
             return strcasecmp(e.get().state.c_str(), state.c_str()) != 0;
           }),
           pl.cend());

  return result;
}

// Helpers implementations
/******************************************************************************/

inline std::string& trimRight(std::string& s) {
  const auto trimEnd =
    std::find_if_not(s.crbegin(), s.crend(), [](const unsigned char &c) {
      return std::isspace(c);
    }).base();
  s.erase(trimEnd, s.end());
  return s;
}

inline std::string removeLastWord(std::string& s) {
  trimRight(s);
  const auto fstSpace =
    std::find_if(s.crbegin(), s.crend(), [](auto &c) {
      return std::isspace(c);
  }).base();
  std::string word(fstSpace, s.cend());
  s.erase(fstSpace, s.end());
  trimRight(s);
  return word;
}

CityRecord cityRecordFromLine(const std::string &line) {
  if (line.size() != 164) {
    throw std::invalid_argument(
      "Places file is mangled, each line needs to be 164 chars"
    );
  }

  std::string name = line.substr(9, 64);  // Name - 9 to 73    len = 64
  const std::string lastWord = removeLastWord(name);
  if (lastWord == "CDP") return {};

  name.shrink_to_fit();

  return CityRecord{
    std::move(name),
    line.substr(0, 2),                    // State- 0 to 2     len = 2
    { 
      std::stod(line.substr(143, 10)),    // Lat  - 143 to 153 len = 10
      std::stod(line.substr(153, 10))     // Long - 153 to 164 len = 10
    }
  };
}

TPlaceRecs loadPlacesFromFile(const char *fName, const size_t approxCount) {
  std::ifstream placesFile(fName);

  if (!placesFile) {
    throw std::invalid_argument(
      "Unable to open " + std::string(fName) + " for reading.");
  }

  auto places = std::make_unique<std::vector<CityRecord>>();
  auto &pl = *places;
  pl.reserve(approxCount);

  std::string line;
  while (std::getline(placesFile, line)) {
    pl.emplace_back(cityRecordFromLine(line));
    if (pl.back().cityName.empty()) pl.pop_back();
  }
  pl.shrink_to_fit();

  std::sort(pl.begin(), pl.end(),
            [](const CityRecord &a, const CityRecord &b) {
              int cmp = strcasecmp(a.cityName.c_str(), b.cityName.c_str());
              return cmp == 0
                ? strcasecmp(a.state.c_str() , b.state.c_str()) < 0
                : cmp < 0;
            });

  return places;
}

// Implementations of the Trie memebers
/******************************************************************************/

PlacesTrie::PlacesTrie(TPlaceRecs cityRecords) :
  places(std::move(cityRecords)), root(0) {
  construct(0, places->size(), 0, root);
}

TrieQueryResult 
PlacesTrie::query(const std::string &cityName) const {
  return query(cityName, root, 0);
}

size_t PlacesTrie::size() const { return places->size(); }

PlacesTrie::TrieNode::TrieNode(const char ch): c(ch) { }

TrieQueryResult PlacesTrie::query(const std::string &cname,
                                          const TrieNode &node,
                                          const int depth) const {
  // Base case, search string exhausted
  if ((int)cname.size() == depth) return getFirstCompletion(node);

  // Binary search on the next nodes to see if next char is in trie
  const char c = std::tolower(cname[depth]);
  const auto it = std::lower_bound(node.next.begin(), node.next.end(), c,
                                   [](const TrieNode &tn, const char ch) {
                                     return tn.c < ch;
                                   });

  // Return empty result when not found
  if (c != it->c)
    return TrieQueryResult{TFoundPlaces(places->cend(), places->cend()), false};

  // Continue searching at next depth
  return query(cname, *it, depth + 1);
}

TrieQueryResult PlacesTrie::getFirstCompletion(
  const TrieNode &node) const {
  // Return the range of records stored in this node when nonempty
  if (node.idxRange.first != -1) {
    return TrieQueryResult{
      TFoundPlaces(places->cbegin() + node.idxRange.first,
                   places->cbegin() + node.idxRange.second),
      false
    };
  }

  // Return not found when last node (shouldn't happen if constructed right)
  if (node.next.empty()) {
    return TrieQueryResult{TFoundPlaces(places->cend(), places->cend()), false};
  }

  // Return the empty sentinel when this is the last node or is ambiguous
  if (node.next.size() > 1) {
    return getAmbiguousHints(node);
  }

  // Continue searching the rest of the trie chain
  return getFirstCompletion(node.next[0]);
}

TrieQueryResult 
PlacesTrie::getAmbiguousHints(const TrieNode &node) const {
  // Find leftmost of matched prefix
  const TrieNode *curr = &node.next.front();
  while (curr->idxRange.first == -1) {
    // Check in case tree not properly constructed
    if (curr->next.empty())
      return TrieQueryResult{TFoundPlaces(places->cend(), places->cend()), false};
    curr = &curr->next.front();
  }
  const int idxLeft = curr->idxRange.first;

  // Find rightmost of matched prefix
  curr = &node.next.back();
  while (curr->idxRange.first == -1) {
    // Check in case tree not properly constructed
    if (curr->next.empty())
      return TrieQueryResult{TFoundPlaces(places->cend(), places->cend()), false};
    curr = &curr->next.back();
  }
  const int idxRight = curr->idxRange.first;

  return TrieQueryResult{
    TFoundPlaces(places->cbegin() + idxLeft,
                 places->cbegin() + idxRight),
    true
  };
}

void PlacesTrie::construct(const int begin, const int end,
                           const int depth, TrieNode &node) const {
  // Start of the current subrange being constructed
  int idx = begin;
  while (idx < end) {
    // End of the subrange being constructed
    const int nextEnd = endOfSameLetterRange(idx, end, depth);

    const char c = std::tolower((*places)[idx].cityName[depth]);
    if (c == '\0') {
      // Save the range of entries with same value
      node.idxRange = {idx, nextEnd};
    } else {
      node.next.emplace_back(c);
      construct(idx, nextEnd, depth + 1, node.next.back());
    }

    // Continue building the next chunk of the subrange
    idx = nextEnd;
  }
}

int PlacesTrie::endOfSameLetterRange(const int fm, const int to,
                                     const size_t depth) const {
  const char c = std::tolower((*places)[fm].cityName[depth]);
  for (int i = fm + 1; i < to; ++i) {
    if (std::tolower((*places)[i].cityName[depth]) != c)
      return i;
  }
  return to;
}
