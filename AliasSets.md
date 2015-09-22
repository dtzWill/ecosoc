#how to use the alias set pass

# Introduction #

The Alias Set Pass provides an interface for the Pointer Analysis implemented in the addr-leaks project. A link to its wiki: http://code.google.com/p/addr-leaks/wiki/HowToUseThePointerAnalysis.

I have uploaded the Pointer Analysis code to this repository, as small changes were made to it.


# Details #

  * To use the pass, it is necessary to correct the includes in lines 12 and 13 of AliasSets.h (or create a link from llvm folder to its real directory).

  * The class implemented provides four main methods:
```
   std::map< int, std::set<Value*> > getValueSets();

   std::map< int, std::set<int> > getMemSets();

   int getValueSetKey(Value* v);

   int getMapSetKey (int m);
```

An alias set is identified by its key, e.g., the alias set number 0 consists of the Value`*` sets obtained with:
```
std::set<Value*> = getValueSets()[0]; 
```

and also of the int values that represent memory positions, obtained with:
```
std::set<int> = getMemSets()[0];
```

The keys for this map has no special meaning, but making the connections between the memory sets and Value`*` sets.

# Example of use #
```
opt -load /path/to/the/compiled/PADriver.so -pa -load /path/to/the/compiled
/AliasSets.so -alias-sets file.bc
```

If the -debug directive is added, the alias sets are printed on the screen.