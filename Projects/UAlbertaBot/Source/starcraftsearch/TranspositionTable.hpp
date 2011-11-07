#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <vector>

template <class HashType, class FType>
class TranspositionTable {

	// the transposition table data structure
	// each pair stores the f-value, and a secondary hash
	std::vector< std::pair<FType, HashType> > TT;

	HashType size;

	int collisions;
	int found, notFound;

	HashType getIndex(HashType hash1) 
	{
		if (hash1 < 0)
		{
			hash1 *= -1;
		} 
		
		return hash1 % size; 
	}

public:

	TranspositionTable<HashType,FType> (HashType size) : size(size), collisions(0), found(0), notFound(0)
	{
		clear();
	}

    TranspositionTable<HashType,FType> () : size(1), collisions(0), found(0), notFound(0)
	{
		clear();
	}
        
	~TranspositionTable<HashType, FType> () {}

	// clear the hash table
	void clear() 
	{
		TT.clear();
		TT = std::vector< std::pair<FType, HashType> >(size, std::pair<FType, HashType>(-1,-1));
	}

	void save(HashType hash1, HashType hash2, FType fvalue) 
	{
		// the index of where this should be stored
		int index = getIndex(hash1);
		
		// only store it if the f-value is greater than the stored value
		if (fvalue >= TT[index].first)
		{
			TT[index].first = fvalue;	
			TT[index].second = hash2;
		}
	}

	// look up a value in the transposition table
	FType lookup(HashType hash1, HashType hash2) 
	{
		int index = getIndex(hash1);
	
		// if there is something here, but it doesn't match hash2
		if (TT[index].first != -1 && TT[index].second != hash2) 
		{
			// it is a collision
			collisions++;
            return -1;
		}	
		// if there is nothing here match, return -1
		else if (TT[index].second != hash2) 
		{ 
			notFound++;
			return -1; 
		}
		else
		{
			found++;
			// otherwise return the value
			return TT[index].first;
		}
	}
	
	int numFound()
	{
		return found;
	}
	
	int numNotFound()
	{
		return notFound;
	}
	
	int numCollisions()
	{
		return collisions;
	}
};

#endif
