#include <stdexcept>
#include <random>
#include <iostream>

#ifndef __MAP_HPP__
#define __MAP_HPP__

#define DEBUG 0

#define SKIP_LIST_LVLS 32

namespace cs540 {
    template <typename _KeyT, typename _MapT>
    class Map {
        struct SkipNode;
        public:
            class Iterator;
            class ConstIterator;
            class ReverseIterator;

            typedef std::pair<const _KeyT, _MapT> _ValT;

            // constructors and assignment operator
            Map();
            Map(const Map &);
            Map& operator=(const Map &);
            Map(std::initializer_list<std::pair<const _KeyT, _MapT>>);
            ~Map();

            // size
            size_t size() const;
            bool empty() const;

            // iterators
            Iterator begin();
            Iterator end();
            ConstIterator begin() const;
            ConstIterator end() const;
            ReverseIterator rbegin();
            ReverseIterator rend();

            // element access
            Iterator find(const _KeyT &);
            ConstIterator find(const _KeyT &) const;
            _MapT &at(const _KeyT &);
            const _MapT &at(const _KeyT &) const;
            _MapT &operator[](const _KeyT &);

            // modifiers
            std::pair<Iterator, bool> insert(const _ValT &);
            template<typename _IterT> void insert(_IterT, _IterT);

            void erase(Iterator);
            void erase(const _KeyT &);
            void clear();

            // comparison
            bool operator==(const Map &);
            bool operator!=(const Map &);
            bool operator<(const Map &);

            // debug
            void debug();

            class Iterator {
                public:
                    //Iterator() = delete;
                    Iterator() = delete;
                    Iterator(SkipNode *r);

                    // prefix
                    Iterator& operator++();
                    Iterator& operator--();
                    // postfix
                    Iterator operator++(int);
                    Iterator operator--(int);

                    _ValT &operator*() const;
                    _ValT *operator->() const;

                    SkipNode *ref = NULL;

                    bool operator==(const Iterator &rhs) { return ref == rhs.ref; }
                    bool operator==(const ConstIterator &rhs) { return ref == rhs.ref; }
                    bool operator!=(const Iterator &rhs) { return ref != rhs.ref; }
                    bool operator!=(const ConstIterator &rhs) { return ref != rhs.ref; }
            };

            class ConstIterator : public Iterator {
                public:
                    using Iterator::Iterator;
                    using Iterator::ref;
                    ConstIterator(const Iterator &);

                    const _ValT &operator*() const;
                    const _ValT *operator->() const;

                    bool operator==(const Iterator &rhs) { return this->ref == rhs.ref; }
                    bool operator==(const ConstIterator &rhs) { return this->ref == rhs.ref; }
                    bool operator!=(const ConstIterator &rhs) { return this->ref != rhs.ref; }
                    bool operator!=(const Iterator &rhs) { return this->ref != rhs.ref; }
            };

            class ReverseIterator : public Iterator {
                public:
                    using Iterator::Iterator;

                    ReverseIterator& operator++();
                    ReverseIterator& operator--();
                    ReverseIterator operator++(int);
                    ReverseIterator operator--(int);

                    bool operator==(const ReverseIterator &rhs) { return this->ref == rhs.ref; }
                    bool operator!=(const ReverseIterator &rhs) { return this->ref != rhs.ref; }
            };

        private:
            struct SkipNode {
                SkipNode(){};
                SkipNode(std::pair<_KeyT, _MapT> p) {
                    value = new _ValT(p);
                }
                SkipNode(const SkipNode &s) { value = new _ValT(*s.value); }
                ~SkipNode() { if (value) delete value; }
                SkipNode &operator=(const SkipNode &s) {
                    if (value) delete value;
                    value = new _ValT(*s.value);
                    return *this;
                }

                _ValT *value = NULL;
                bool end = false, begin = false;
                SkipNode *prev = NULL,
                         *next = NULL,
                         *above = NULL,
                         *below = NULL;
            };

            // probability generator
            std::random_device rd{};
            std::mt19937 mt = std::mt19937(rd());
            std::uniform_int_distribution<unsigned int> dist{0, 1};

            // pointers
            SkipNode *head = NULL;
            SkipNode *bottomHead = NULL;
            SkipNode *bottomTail = NULL;
            size_t sz = 0;
    };

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::Map() {
        head = new SkipNode();
        head->begin = true;

        SkipNode *curr = head;
        for (int i = 1; i < SKIP_LIST_LVLS; i++) {
            SkipNode *temp = new SkipNode;
            temp->begin = true;
            curr->below = temp;
            temp->above = curr;
            curr = temp;
        }
        bottomHead = curr;

        SkipNode *sentinel = new SkipNode;
        bottomTail = sentinel;
        sentinel->end = true;
        bottomHead->next = sentinel;
        sentinel->prev = bottomHead;
    }

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::Map(const Map &m) {
        head = new SkipNode;

        SkipNode *rightMostNodes[SKIP_LIST_LVLS];
        rightMostNodes[SKIP_LIST_LVLS-1] = head;

        SkipNode *curr = head;
        for (int i = SKIP_LIST_LVLS-2; i >= 0; i--) {
            SkipNode *temp = new SkipNode;
            curr->below = temp;
            temp->above = curr;
            curr = temp;
            rightMostNodes[i] = curr;
        }
        bottomHead = curr;
        SkipNode *sentinel = new SkipNode;
        bottomTail = sentinel;
        sentinel->end = true;
        bottomHead->next = sentinel;
        sentinel->prev = bottomHead;
        sz = m.sz;

        if (m.sz) { // if any elements in the copy list
            curr = m.bottomHead->next;
            // traverse through bottom layer
            while (curr && !curr->end) {
                int currentLevel = 0;
                SkipNode *copyNode = new SkipNode;
                // deep copy
                copyNode->end = curr->end;
                copyNode->value = new _ValT(*(curr->value));

                // link horizontally
                copyNode->prev = rightMostNodes[currentLevel];
                copyNode->next = rightMostNodes[currentLevel]->next;
                rightMostNodes[currentLevel]->next = copyNode;

                // set history
                rightMostNodes[currentLevel] = copyNode;

                // traverse vertically...
                SkipNode * prevVert = copyNode;
                SkipNode *vertCurr = curr->above;
                while (vertCurr) {
                    // keep track of what level
                    currentLevel++;

                    // deep copy
                    SkipNode *vertCopyNode = new SkipNode;
                    vertCopyNode->value = new _ValT(*(vertCurr->value));

                    // link horizontally
                    vertCopyNode->prev = rightMostNodes[currentLevel];
                    rightMostNodes[currentLevel]->next = vertCopyNode;

                    // link vertically
                    prevVert->above = vertCopyNode;
                    vertCopyNode->below = prevVert;

                    // set history
                    rightMostNodes[currentLevel] = vertCopyNode;

                    // move up
                    prevVert = vertCopyNode;
                    vertCurr = vertCurr->above;
                }
                curr = curr->next;
            }
            rightMostNodes[0]->next->prev = rightMostNodes[0];
        }
    }

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>& Map<_KeyT, _MapT>::operator=(const Map &m) {
        if (this != &m) {
            clear();

            // history nodes
            SkipNode *rightMostNodes[SKIP_LIST_LVLS];
            // fill backwards to keep consistent with levels
            rightMostNodes[SKIP_LIST_LVLS-1] = head;

            SkipNode *curr = head;
            for (int i = SKIP_LIST_LVLS-2; i >= 0; i--) {
                curr = curr->below;
                rightMostNodes[i] = curr;
            }
            bottomHead = curr;

            sz = m.sz;
            if (m.sz) {
                curr = m.bottomHead->next;
                while (curr && !curr->end) {
                    int currentLevel = 0;
                    SkipNode *copyNode = new SkipNode;
                    // deep copy
                    copyNode->end = curr->end;
                    copyNode->value = new _ValT(*(curr->value));

                    // link horizontally
                    copyNode->prev = rightMostNodes[currentLevel];
                    copyNode->next = rightMostNodes[currentLevel]->next;
                    rightMostNodes[currentLevel]->next = copyNode;

                    // set history
                    rightMostNodes[currentLevel] = copyNode;

                    // traverse vertically...
                    SkipNode * prevVert = copyNode;
                    SkipNode *vertCurr = curr->above;
                    while (vertCurr) {
                        // keep track of what level
                        currentLevel++;

                        SkipNode *vertCopyNode = new SkipNode;
                        // deep copy
                        vertCopyNode->value = new _ValT(*(vertCurr->value));

                        // link horizontally
                        vertCopyNode->prev = rightMostNodes[currentLevel];
                        rightMostNodes[currentLevel]->next = vertCopyNode;

                        // link vertically
                        prevVert->above = vertCopyNode;
                        vertCopyNode->below = prevVert;

                        // set history
                        rightMostNodes[currentLevel] = vertCopyNode;

                        // move up
                        prevVert = vertCopyNode;
                        vertCurr = vertCurr->above;
                    }
                    curr = curr->next;
                }
                rightMostNodes[0]->next->prev = rightMostNodes[0];
            }
        }
        return *this;
    }

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::Map(std::initializer_list<std::pair<const _KeyT, _MapT>> il) {
        head = new SkipNode();
        head->begin = true;

        SkipNode *curr = head;
        for (int i = 1; i < SKIP_LIST_LVLS; i++) {
            SkipNode *temp = new SkipNode;
            temp->begin = true;
            curr->below = temp;
            temp->above = curr;
            curr = temp;
        }
        bottomHead = curr;

        SkipNode *sentinel = new SkipNode;
        bottomTail = sentinel;
        sentinel->end = true;
        bottomHead->next = sentinel;
        sentinel->prev = bottomHead;

        for (auto &e : il) {
            insert(e);
        }
    }

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::~Map() {
        SkipNode *currHeader = head;

        while (currHeader) {
            if (currHeader->next) {
                SkipNode *curr = currHeader->next;
                while (curr) {
                    SkipNode *temp = curr;
                    curr = curr->next;
                    delete temp;
                }
            }
            SkipNode *temp = currHeader;
            currHeader = currHeader->below;
            delete temp;
        }
    }

    template <typename _KeyT, typename _MapT>
    size_t Map<_KeyT, _MapT>::size() const {
        return sz;
    }

    template <typename _KeyT, typename _MapT>
    bool Map<_KeyT, _MapT>::empty() const {
        return (sz) ? false : true;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator Map<_KeyT, _MapT>::begin() {
        return Iterator(bottomHead->next);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator Map<_KeyT, _MapT>::end() {
        return Iterator(bottomTail);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ConstIterator Map<_KeyT, _MapT>::begin() const {
        return ConstIterator(bottomHead->next);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ConstIterator Map<_KeyT, _MapT>::end() const {
        return ConstIterator(bottomTail);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator Map<_KeyT, _MapT>::rbegin() {
        return ReverseIterator(bottomTail->prev);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator Map<_KeyT, _MapT>::rend() {
        return ReverseIterator(bottomHead);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator Map<_KeyT, _MapT>::find(const _KeyT &k) {
        SkipNode *curr = head;
        while (!(curr->end)) {
            if (!(curr->begin) && !(curr->end) && curr->value->first == k) break;

            if (curr->next) {
                if (!curr->next->end && (curr->next->value->first < k || curr->next->value->first == k)) {
                    curr = curr->next;
                } else if (curr->below) {
                    curr = curr->below;
                } else {
                    return end();
                }
            } else if (curr->below) {
                curr = curr->below;
            } else {
                return end();
            }
        }

        while (curr->below) curr = curr->below;
        return Iterator(curr);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ConstIterator Map<_KeyT, _MapT>::find(const _KeyT &k) const {
        SkipNode *curr = head;
        while (!(curr->end)) {
            if (!(curr->begin) && !(curr->end) && curr->value->first == k) break;

            if (curr->next) {
                if (curr->next->value->first < k || curr->next->value->first == k) {
                    curr = curr->next;
                } else if (curr->below) {
                    curr = curr->below;
                } else {
                    return end();
                }
            } else if (curr->below) {
                curr = curr->below;
            } else {
                return end();
            }
        }

        while (curr->below) curr = curr->below;
        return ConstIterator(curr);
    }

    template <typename _KeyT, typename _MapT>
    _MapT &Map<_KeyT, _MapT>::at(const _KeyT &k) {
        Iterator search = find(k);
        if (search != end()) {
            return search->second;
        } else {
            throw std::out_of_range("Map<>::at : Could not find specified key in map.");
        }
    }

    template <typename _KeyT, typename _MapT>
    const _MapT &Map<_KeyT, _MapT>::at(const _KeyT &k) const {
        ConstIterator search = find(k);
        if (search != end()) {
            return search->second;
        } else {
            throw std::out_of_range("const Map<>::at : Could not find specified key in map.");
        }
    }

    template <typename _KeyT, typename _MapT>
    _MapT &Map<_KeyT, _MapT>::operator[](const _KeyT &k) {
        Iterator search = find(k);
        if (search != end()) {
            return search->second;
        } else {
            auto ret = insert({k, _MapT{}});
            return ret.first->second;
        }
    }

    template <typename _KeyT, typename _MapT>
    std::pair<typename Map<_KeyT, _MapT>::Iterator, bool> Map<_KeyT, _MapT>::insert(const _ValT &elem) {
        int currLevel = 31;
        SkipNode *history[SKIP_LIST_LVLS];
        SkipNode *curr = head;
        while (curr) {
            if (!(curr->end) && !(curr->begin) && curr->value->first == elem.first) {
                while (curr->below) curr = curr->below;
                return std::pair<Iterator, bool>{Iterator(curr), false};
            }

            if (!(curr->next)) {
                history[currLevel--] = curr;
                curr = curr->below;
            } else if (!curr->next->end && (curr->next->value->first < elem.first || curr->next->value->first == elem.first)) {
                curr = curr->next;
            } else {
                history[currLevel--] = curr;
                curr = curr->below;
            }
        }

        SkipNode *insertNode = new SkipNode(elem);
        auto ret = std::pair<Iterator, bool>{Iterator(insertNode), true};
        insertNode->next = history[0]->next;
        insertNode->prev = history[0];
        if (insertNode->next) insertNode->next->prev = insertNode;
        history[0]->next = insertNode;

        int coinFlip, insertLevel = 0;
        while ((coinFlip = dist(mt))) {
            insertLevel++;
            if (insertLevel > (SKIP_LIST_LVLS-1)) break;
        }

        SkipNode *previousInsert = insertNode;
        for (int i = 1; i <= insertLevel; i++) {
            insertNode = new SkipNode(elem);
            insertNode->next = history[i]->next;
            insertNode->prev = history[i];
            if (insertNode->next) insertNode->next->prev = insertNode;
            history[i]->next = insertNode;

            previousInsert->above = insertNode;
            insertNode->below = previousInsert;
            previousInsert = insertNode;
        }

        sz++;
        return ret;
    }

    template <typename _KeyT, typename _MapT>
    template <typename _IterT>
    void Map<_KeyT, _MapT>::insert(_IterT begin, _IterT end) {
        for (; begin != end; begin++) {
            insert(*begin);
        }
    }

    template <typename _KeyT, typename _MapT>
    void Map<_KeyT, _MapT>::erase(Iterator pos) {
        SkipNode *curr = pos.ref;
        while (curr) {
            curr->prev->next = curr->next;
            if (curr->next) curr->next->prev = curr->prev;
            SkipNode *temp = curr;
            curr = curr->above;
            delete temp;
        }
        sz--;
    }

    template <typename _KeyT, typename _MapT>
    void Map<_KeyT, _MapT>::erase(const _KeyT &k) {
        SkipNode *curr = head;
        while (!curr->end) {
            if (!(curr->begin) && !(curr->end) && curr->value->first == k) {
                while (curr) {
                    SkipNode *temp = curr->below;
                    curr->prev->next = curr->next;
                    if (curr->next) curr->next->prev = curr->prev;
                    delete curr;
                    curr = temp;
                }
                sz--;
                return;
            }

            if (curr->next) {
                if (!curr->next->end && (curr->next->value->first < k || curr->next->value->first == k)) {
                    curr = curr->next;
                } else if (curr->below) {
                    curr = curr->below;
                } else {
                    throw std::out_of_range("Map<>::erase : Could not find specified key in map.");
                }
            } else if (curr->below) {
                curr = curr->below;
            } else {
                throw std::out_of_range("Map<>::erase : Could not find specified key in map.");
            }
        }

        throw std::out_of_range("Map<>::erase : Could not find specified key in map.");
    }

    template <typename _KeyT, typename _MapT>
        void Map<_KeyT, _MapT>::clear() {
            SkipNode *curr = bottomHead->next;
            while (curr && !curr->end) {
                SkipNode * temp;
                SkipNode * vertCurr = curr->above;
                while (vertCurr) {
                    temp = vertCurr;
                    vertCurr = vertCurr->above;
                    delete temp;
                }
                temp = curr;
                curr = curr->next; sz--;
                delete temp;
            }
            SkipNode *tempSent = curr;
            // reset rowHeader->next pointers
            curr = head; 
            while (curr) {
                curr->next = NULL;
                curr = curr->below;
            }
            bottomHead->next = tempSent;
            tempSent->prev = bottomHead;
    }

    template <typename _KeyT, typename _MapT>
    bool Map<_KeyT, _MapT>::operator==(const Map &rhs) {
        if (sz == rhs.sz) {
            SkipNode *curr = bottomHead->next;
            SkipNode *rhsCurr = rhs.bottomHead->next;
            while (!(curr->end)) {
                if (*curr->value != *rhsCurr->value) return false;
                curr = curr->next;
                rhsCurr = rhsCurr->next;
            }
            return true;
        } else {
            return false;
        }
    }

    template <typename _KeyT, typename _MapT>
    bool Map<_KeyT, _MapT>::operator!=(const Map &rhs) {
        return !(*this == rhs);
    }

    template <typename _KeyT, typename _MapT>
    bool Map<_KeyT, _MapT>::operator<(const Map &rhs) {
        if (sz < rhs.sz) {
            SkipNode *curr = bottomHead->next;
            SkipNode *rCurr = rhs.bottomHead->next;
            bool equal = true;
            while (!(curr->end)) {
                if (*curr->value < *rCurr->value) return true;
                if (*curr->value != *rCurr->value) equal = false;
                curr = curr->next;
                rCurr = rCurr->next;
            }
            if (equal) return true;
            else return false;
        } else {
            return false;
        }
    }

    /*
     * ITERATOR
     */

    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::Iterator::Iterator(SkipNode *r) {
        ref = r;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator &Map<_KeyT, _MapT>::Iterator::operator++() {
        ref = ref->next;
        return *this;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator &Map<_KeyT, _MapT>::Iterator::operator--() {
        ref = ref->prev;
        return *this;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator Map<_KeyT, _MapT>::Iterator::operator++(int) {
        Iterator ret(ref);
        ref = ref->next;
        return ret;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::Iterator Map<_KeyT, _MapT>::Iterator::operator--(int) {
        Iterator ret(ref);
        ref = ref->prev;
        return ret;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::_ValT &Map<_KeyT, _MapT>::Iterator::operator*() const {
        return *(ref->value);
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::_ValT *Map<_KeyT, _MapT>::Iterator::operator->() const {
        return ref->value;
    }

    /*
     * CONST_ITERATOR
     */
    template <typename _KeyT, typename _MapT>
    Map<_KeyT, _MapT>::ConstIterator::ConstIterator(const Iterator &i) : Iterator(i.ref) {}

    template <typename _KeyT, typename _MapT>
    const typename Map<_KeyT, _MapT>::_ValT &Map<_KeyT, _MapT>::ConstIterator::operator*() const {
        return *(this->ref->value);
    }

    template <typename _KeyT, typename _MapT>
    const typename Map<_KeyT, _MapT>::_ValT *Map<_KeyT, _MapT>::ConstIterator::operator->() const {
        return this->ref->value;
    }

    /*
     * REVERSE_ITERATOR
     */
    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator &Map<_KeyT, _MapT>::ReverseIterator::operator++() {
        this->ref = this->ref->prev;
        return *this;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator &Map<_KeyT, _MapT>::ReverseIterator::operator--() {
        this->ref = this->ref->next;
        return *this;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator Map<_KeyT, _MapT>::ReverseIterator::operator++(int) {
        ReverseIterator ret(this->ref);
        this->ref = this->ref->prev;
        return ret;
    }

    template <typename _KeyT, typename _MapT>
    typename Map<_KeyT, _MapT>::ReverseIterator Map<_KeyT, _MapT>::ReverseIterator::operator--(int) {
        ReverseIterator ret(this->ref);
        this->ref = this->ref->next;
        return ret;
    }
}

#endif
