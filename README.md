# Bimap

bimap is a data structure that discovers a set of pairs and efficiently looks up a key of choice. Unlike a map, a search in a bimap can be performed both on the left (left) element and on the right (right). Class has 4 template parameters 2 of which is comparators for types. Cartesian tree is used as storage method, this provides fast operations of find, insert and erase O(log(n)). 



