#pragma once

// 平台无关的不稳定排序：逐步复刻 MSVC STL 的 std::sort。
//
// std::sort 不稳定，相等元素排完落在哪儿由实现决定——MSVC 的 STL 与 libc++ 不一样。
// 背包按年份重排（GameState::removeItem）时绝大多数东西年份都是 0，谁先谁后全凭
// 实现，而背包次序是写进存档的：第 7 章起背包超过 32 格，MSVC 走快排那一支，
// 在 macOS 上用 libc++ 排出来的存档与 Windows 上的交接存档逐行对不上。
// 所以这里照搬 MSVC 的算法（插入排序阈值 32、Tukey 九数取中、三路划分、
// 1.5 log2(N) 层后改堆排序），让两边排出同一个次序、历史存档一字不改。
//
// 来源：microsoft/STL 的 stl/inc/algorithm（_Sort_unchecked、_Insertion_sort_unchecked、
// _Med3_unchecked、_Guess_median_unchecked、_Partition_by_pivot_unchecked）与
// stl/inc/__msvc_heap_algorithms.hpp（堆排序那一支）。
// Copyright (c) Microsoft Corporation. SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <algorithm>
#include <iterator>
#include <utility>

namespace fanren::core::portable_sort_detail {

inline constexpr long kInsertionSortMax = 32;

template <class It, class Pred>
void insertionSort(It first, It last, Pred pred) {
    if (first == last) return;
    for (It mid = first; ++mid != last;) {
        It hole = mid;
        auto val = std::move(*mid);
        if (pred(val, *first)) {
            std::move_backward(first, mid, ++hole);
            *first = std::move(val);
        } else {
            for (It prev = hole; pred(val, *--prev); hole = prev) {
                *hole = std::move(*prev);
            }
            *hole = std::move(val);
        }
    }
}

template <class It, class Pred>
void med3(It first, It mid, It last, Pred pred) {
    using std::swap;
    if (pred(*mid, *first)) swap(*mid, *first);
    if (pred(*last, *mid)) {
        swap(*last, *mid);
        if (pred(*mid, *first)) swap(*mid, *first);
    }
}

template <class It, class Pred>
void guessMedian(It first, It mid, It last, Pred pred) {
    using Diff = typename std::iterator_traits<It>::difference_type;
    const Diff count = last - first;
    if (40 < count) {   // Tukey's ninther
        const Diff step = (count + 1) >> 3;
        const Diff twoStep = step << 1;
        med3(first, first + step, first + twoStep, pred);
        med3(mid - step, mid, mid + step, pred);
        med3(last - twoStep, last - step, last, pred);
        med3(first + step, mid, last - step, pred);
    } else {
        med3(first, mid, last, pred);
    }
}

template <class It, class Pred>
std::pair<It, It> partitionByPivot(It first, It pfirst, It last, Pred pred) {
    using std::swap;
    It plast = std::next(pfirst);

    while (first < pfirst && !pred(*std::prev(pfirst), *pfirst) && !pred(*pfirst, *std::prev(pfirst))) {
        --pfirst;
    }
    while (plast < last && !pred(*plast, *pfirst) && !pred(*pfirst, *plast)) {
        ++plast;
    }

    It gfirst = plast;
    It glast = pfirst;

    for (;;) {
        for (; gfirst < last; ++gfirst) {
            if (pred(*pfirst, *gfirst)) {
                continue;
            } else if (pred(*gfirst, *pfirst)) {
                break;
            } else if (plast != gfirst) {
                swap(*plast, *gfirst);
                ++plast;
            } else {
                ++plast;
            }
        }

        for (; first < glast; --glast) {
            const It glastPrev = std::prev(glast);
            if (pred(*glastPrev, *pfirst)) {
                continue;
            } else if (pred(*pfirst, *glastPrev)) {
                break;
            } else if (--pfirst != glastPrev) {
                swap(*pfirst, *glastPrev);
            }
        }

        if (glast == first && gfirst == last) {
            return {pfirst, plast};
        }

        if (glast == first) {   // no room at bottom, rotate pivot upward
            if (plast != gfirst) swap(*pfirst, *plast);
            ++plast;
            swap(*pfirst, *gfirst);
            ++pfirst;
            ++gfirst;
        } else if (gfirst == last) {   // no room at top, rotate pivot downward
            if (--glast != --pfirst) swap(*glast, *pfirst);
            swap(*pfirst, *--plast);
        } else {
            swap(*gfirst, *--glast);
            ++gfirst;
        }
    }
}

template <class It, class Diff, class T, class Pred>
void pushHeapByIndex(It first, Diff hole, Diff top, T&& val, Pred pred) {
    for (Diff idx = (hole - 1) >> 1; top < hole && pred(*(first + idx), val); idx = (hole - 1) >> 1) {
        *(first + hole) = std::move(*(first + idx));
        hole = idx;
    }
    *(first + hole) = std::forward<T>(val);
}

template <class It, class Diff, class T, class Pred>
void popHeapHoleByIndex(It first, Diff hole, Diff bottom, T&& val, Pred pred) {
    const Diff top = hole;
    Diff idx = hole;
    const Diff maxNonLeaf = (bottom - 1) >> 1;
    while (idx < maxNonLeaf) {
        idx = 2 * idx + 2;
        if (pred(*(first + idx), *(first + (idx - 1)))) --idx;
        *(first + hole) = std::move(*(first + idx));
        hole = idx;
    }
    if (idx == maxNonLeaf && bottom % 2 == 0) {
        *(first + hole) = std::move(*(first + (bottom - 1)));
        hole = bottom - 1;
    }
    pushHeapByIndex(first, hole, top, std::forward<T>(val), pred);
}

template <class It, class Pred>
void makeHeap(It first, It last, Pred pred) {
    using Diff = typename std::iterator_traits<It>::difference_type;
    const Diff bottom = last - first;
    for (Diff hole = bottom >> 1; hole > 0;) {
        --hole;
        auto val = std::move(*(first + hole));
        popHeapHoleByIndex(first, hole, bottom, std::move(val), pred);
    }
}

template <class It, class Pred>
void sortHeap(It first, It last, Pred pred) {
    using Diff = typename std::iterator_traits<It>::difference_type;
    for (; last - first >= 2; --last) {
        It back = std::prev(last);
        auto val = std::move(*back);
        *back = std::move(*first);
        popHeapHoleByIndex(first, Diff{0}, static_cast<Diff>(back - first), std::move(val), pred);
    }
}

template <class It, class Diff, class Pred>
void sortUnchecked(It first, It last, Diff ideal, Pred pred) {
    for (;;) {
        if (last - first <= kInsertionSortMax) {
            insertionSort(first, last, pred);
            return;
        }
        if (ideal <= 0) {
            makeHeap(first, last, pred);
            sortHeap(first, last, pred);
            return;
        }
        It mid = first + ((last - first) >> 1);
        guessMedian(first, mid, std::prev(last), pred);
        const std::pair<It, It> split = partitionByPivot(first, mid, last, pred);

        ideal = (ideal >> 1) + (ideal >> 2);   // allow 1.5 log2(N) divisions

        if (split.first - first < last - split.second) {
            sortUnchecked(first, split.first, ideal, pred);
            first = split.second;
        } else {
            sortUnchecked(split.second, last, ideal, pred);
            last = split.first;
        }
    }
}

}  // namespace fanren::core::portable_sort_detail

namespace fanren::core {

// 与 MSVC 的 std::sort(first, last, pred) 逐次比较、逐次交换相同。
template <class It, class Pred>
void portableSort(It first, It last, Pred pred) {
    portable_sort_detail::sortUnchecked(first, last, last - first, pred);
}

}  // namespace fanren::core
