/*
* Date: 03/07/2014
* File name: MapReduceTest.cpp  
* Author: Ghislain Landry
* Purpose: Voir le README
*/

#include <iostream>
#include <string>
#include <algorithm>
#include <limits>
#include <string.h>   // <--- pour utilse strtok de C
#include <sstream> // <--- pour convertier les string en entier peut se faire avec
			// to_string, mais linker avec c++11
#include <map> 
#include  "stdint.h"  // <--- evite les erreurs du genre uint64_t! 

// hadoop version 1.0.3, 64 bits
#include "/usr/local/hadoop/c++/Linux-amd64-64/include/hadoop/Pipes.hh"
#include "/usr/local/hadoop/c++/Linux-amd64-64/include/hadoop/TemplateFactory.hh"
#include "/usr/local/hadoop/c++/Linux-amd64-64/include/hadoop/StringUtils.hh"


class WordPair {
	
	public:
		WordPair(){}
		WordPair(std::string str1, std::string str2): word1(str1), word2(str2){}
		~WordPair(){}
		std::vector<std::string> getPair();

		std::string toString();

	private:
		std::string word1;
		std::string word2;
};

std::vector<std::string> WordPair::getPair() {
	std::vector<std::string> vect;
	
	vect.push_back(word1);
	vect.push_back(word2);

	return vect;
}

std::string WordPair::toString(){
	return std::string(word1 + "," + word2 );

}

class SimTagsMap: public HadoopPipes::Mapper {

	public:
		SimTagsMap(HadoopPipes::TaskContext& context){}
		/* Parcours un vecteur et supprime  les valeurs identiques*/
                std::vector<std::string> removeDuplicate(std::vector<std::string>& list_val);

		void map(HadoopPipes::MapContext& context);
};

std::vector<std::string> SimTagsMap::removeDuplicate(std::vector<std::string>& list_val){
	std::map<std::string, int> tmp_map;
	for(unsigned int i = 0; i < list_val.size(); ++i){
		if(!tmp_map.count(list_val[i])){
			tmp_map.insert(std::make_pair(list_val[i],1));
		}
	}

	list_val.clear();
	std::map<std::string, int>::iterator it;
	for(it = tmp_map.begin() ; it != tmp_map.end(); ++it){
		list_val.push_back(it->first);
	}
	return list_val;
}

/*
Mapper, l'algorithme est decrit dans le README
*/

void SimTagsMap::map(HadoopPipes::MapContext& context) {

                        //lecture des tags to a post
	std::vector<std::string> tagList =
        HadoopUtils::splitString( context.getInputValue(), " ");

        char *str;
        std::string _string;
        std::vector<std::string> tags;


        for (unsigned int i = 0; i < tagList.size(); i++) {
                str = NULL;
                str = strtok(const_cast<char*> (tagList[i].c_str()), ",");

                tags.clear();

                if(str){
                        tags.push_back(const_cast<char *> (str));

                        str = strtok(NULL, ",");
                        while( str!= NULL) {

                        	tags.push_back(const_cast<char*> (str));
                                //lecture du token suivant
                                str = strtok(NULL, ",");
                        }
			//Suppression des tags identiques dans un meme post
                        tags = removeDuplicate(tags); // supprime les tags identiques lorsqu'ils sont associe a un meme post

                        // Si le post ne contient qu'un seul tag, alors pas de paire a emettre
                        if(tags.size() > 1) {
				// formate un couple de la forme: 
				// c++ -> {(hadoop, 1), (java, 1), (java-script, 1), (xml, 1)}
				// en:
				// hadoop,1:java,1:java-script,1:xml,1
				// ou c++, qui est la cle n'est pas ajoute a la valeur
                                for (unsigned int k = 0; k < tags.size(); ++k) {
                                        // ":" est utilise comme delimiteur dans la chaine a transmettre
                                        _string = "";

                                        for(unsigned r = 0; r < tags.size() -1; ++r) { // on copie tout sauf le dernier element
                                                                                                        // pour eviter le ":" a la fin
                                                if(!tags[k].compare(tags[r])) continue;

                                                WordPair wordPair(tags[r], "1");
                                                _string += wordPair.toString() + ":";

                                        }
                                        if(tags[k].compare(tags[tags.size() -1]) == 0) { // supprime la virgure inseree a la fin
                                                char * substr = const_cast<char*> (_string.c_str());
                                                substr[_string.length()-1] = '\0';
                                                _string = std::string(const_cast<char*> (substr));
                                        }
                                        else {
                                                WordPair wordPair(tags[tags.size() -1], "1");
                                                _string += wordPair.toString();
                                        }
							//on emet une chaine de la forme: (c++,java,1:java-script,1:hadoop,1) ou C++ est la clef
                                                        // et le reste la valeur
                                        context.emit(tags[k], _string);
                                }
                        }
                }
        }

}
							


bool mycompare( std::pair<std::string, int> a, std::pair<std::string, int> b){
                        return a.second > b.second;
}

class SimTagsReduce: public HadoopPipes::Reducer {

	public:
		SimTagsReduce(HadoopPipes::TaskContext& context){}


		void reduce( HadoopPipes::ReduceContext& context);

};

/*
Reducer: l'algorithme est decrit dans le README
*/
void SimTagsReduce::reduce(HadoopPipes::ReduceContext& context) {

	//int value = 0;
        std::map<std::string, int> maptable;
        std::string key, string;
        std::map<std::string, int>::const_iterator it = maptable.begin();
        char *str = NULL;
	std::vector<std::string> string_vect;
        std::stringstream out;

        std::vector< std::pair<std::string,int> > items;

        while(context.nextValue()){
        	//std::string 
		string = context.getInputValue(); //HadoopUtils::toString(context.getInputValue());
                string_vect.clear();
		
		// reconstitution de notre table de de hachage, c'est a dire conversion de:
		// hadoop,1:java,1:java-script,1:xml,1
		// en:
                // c++ -> {(hadoop, 1), (java, 1), (java-script, 1), (xml, 1)}
                // note que la cle est geree par MapReduce

                str = NULL;
                str  = strtok(const_cast<char*> (string.c_str()), ":");
               	while(str){
                	string_vect.push_back(std::string(const_cast<char*> (str)));
                	str = strtok(NULL, ":");
               	}

                for(unsigned int k = 0; k < string_vect.size();  ++k){
                	str  = strtok(const_cast<char*> (string_vect[k].c_str()), ",");
                        std::string s = std::string(const_cast<char*> (str)); //lecture d'un tag
                        str = strtok(NULL, ",");
                       	int numb;
                        std::istringstream ( str ) >> numb; // convertion de la chaine en entier
			// cree une entree dans la table de hachage si le tag n'y est pas 
                        if(!maptable.count(s)){
                       		maptable.insert(std::make_pair(s,numb));
                        }
                        else{
				//ajoute le nombre recu a la valeur de l'entree
                        	maptable.at(s) += numb;
                        }
                }
        }
	// C++ map, ne tri une map que par la cle, nous devons donc copier toutes les paires dans un
	// vecteur afin de le trier par valeur, car on souhaite ne emettre que les 3 tags qui 
	// apparqissent le plus souvent avec le tag en cours de traitement  
	it = maptable.begin();
        items.push_back(*it);
        ++it;

        while(it != maptable.end()){
                items.push_back(*it);

                ++it;
	}
        out.str("");
       	// trie des valeurs pour ne emettre que les trois plus plus grandes valeurs
       	// il s'agit en effet des trois tags qui apparaissent le plus souvent avec
        // le tag qui est la clef
	// la function mycompare definie comment comparer deux paire
        std::sort(items.begin(), items.end(), mycompare);

	// reformatage de la chaine a transmettre conformement a la chaine recue
       	out << items[0].second;
        string = items[0].first + "," + out.str();

      	if(items.size() >= 3){
                for(unsigned int k = 1; k < 3; ++k){ //k commence a 1 car on a deja recuperer une valeur
                	out.str("");
                	out << items[k].second;
                	string += ":" + items[k].first + "," + out.str();
                }
                context.emit(context.getInputKey(), string);
        }
	else{ // la cle a moins de 3 tag, envoie de toutes les autres tags
		unsigned int l = 1;
		while(l < items.size()){
			out.str("");
			out << items[l].second;
			string += ":" + items[l].first + "," + out.str();
			++l;
		}
		context.emit(context.getInputKey(), string);
	}
}



int main (int argc, char* argv[]){

	return HadoopPipes::runTask(HadoopPipes::TemplateFactory<SimTagsMap,
					SimTagsReduce>());
}
