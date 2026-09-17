## Membtable
A skiplist-based memtable implemetation.

The skiplist is reimplemented from William Pugh's work on skiplist reserach from the 1990 paper.

The memtable perfroms insert, search, deletion, immutable conversion and flush operations.

Key's are never deleted / over-written in-place. Instead, deletions insert a sentinel `version` value, ideally a tombstone marker.
