/**
* Programme qui lit le fichier binaire films.bin et le fichier text Livre.txt afin
 * de creer une bibliothèque. La class bilitothèque contient les livres, les films  et
 * les combos LivreFilm sous forme d'Item. Chaque item est affiché grâce à la
 * class abstraite Affichable. De plus, la fonction afficherListeItems() permet l'affichage
 * de plusieurs conteneurs d'Items différents.
* \file td5.cpp
* \author Nyouvop et Haddad
* \date 31 mars 2024
* Créé le 25 mars  2024
*/
#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "classes_td5.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <sstream>
#include <typeinfo>
#include <iterator>
#include <list>
#include <forward_list>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).
using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{
template <typename T>
T lireType(istream& fichier)
{
    T valeur{};
    fichier.read(reinterpret_cast<char*>(&valeur), sizeof(valeur));
    return valeur;
}
#define erreurFataleAssert(message) assert(false&&(message)),terminate()
static const uint8_t enteteTailleVariableDeBase = 0xA0;
size_t lireUintTailleVariable(istream& fichier)
{
    uint8_t entete = lireType<uint8_t>(fichier);
    switch (entete) {
        case enteteTailleVariableDeBase + 0: return lireType<uint8_t>(fichier);
        case enteteTailleVariableDeBase + 1: return lireType<uint16_t>(fichier);
        case enteteTailleVariableDeBase + 2: return lireType<uint32_t>(fichier);
        default:
            erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");  //NOTE: Il n'est pas possible de faire des tests pour couvrir cette ligne en plus du reste du programme en une seule exécution, car cette ligne termine abruptement l'exécution du programme.  C'est possible de la couvrir en exécutant une seconde fois le programme avec un fichier films.bin qui contient par exemple une lettre au début.
    }
}

string lireString(istream& fichier)
{
    string texte;
    texte.resize(lireUintTailleVariable(fichier));
    fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
    return texte;
}



#pragma endregion//}



void Bibliotheque::ajouterItem(unique_ptr<Item> item) {
    items_.push_back(std::move(item));
}


span<unique_ptr<Item>> Bibliotheque::obtenirItems() { return span(items_); }

void Bibliotheque::enleverItem(const unique_ptr<Item>& item) {
    for (int i : range(size())) {  // Doit être une référence au pointeur pour pouvoir le modifier.
        if (items_[i] == item) {
            if (size() > 1)
                items_.erase(items_.begin() + i);
            return;
        }
    }
}


const vector<shared_ptr<Acteur>>& Film::obtenirActeurs() const {
    return acteurs_.obtenirElements();
}



shared_ptr<Acteur> Bibliotheque::trouverActeur(const string& nomActeur) const
{
    for (auto&& item : span(items_)) {
        for (const shared_ptr<Acteur>& acteur : item->obtenirActeurs()) {
            if (acteur->obtenirNom() == nomActeur)
                return acteur;

        }
    }
    return nullptr;
}

shared_ptr<Acteur> lireActeur(istream& fichier, const Bibliotheque& bibliotheque)
{
    Acteur acteur = {};
    acteur.nom_ = lireString(fichier);
    acteur.anneeNaissance_ = int(lireUintTailleVariable(fichier));
    acteur.sexe_ = char(lireUintTailleVariable(fichier));

    shared_ptr<Acteur> acteurExistant = bibliotheque.trouverActeur(acteur.nom_);
    if (acteurExistant != nullptr)
        return acteurExistant;
    else {
        cout << "Création Acteur " << acteur.nom_ << endl;
        return make_shared<Acteur>(std::move(acteur));  // Le move n'est pas nécessaire mais permet de transférer le texte du nom sans le copier.
    }
}

unique_ptr<Film> lireFilm(istream& fichier, Bibliotheque& bibliotheque)
{
    unique_ptr<Film> film = make_unique<Film>();
    film->titre_ = lireString(fichier);
    film->realisateur_ = lireString(fichier);
    film->annee_ = int(lireUintTailleVariable(fichier));
    film->recette_ = int(lireUintTailleVariable(fichier));
    auto nActeurs = int(lireUintTailleVariable(fichier));
    film->acteurs_ = ListeActeurs(nActeurs);  // On n'a pas fait de méthode pour changer la taille d'allocation, seulement un constructeur qui prend la capacité.  Pour que cette affectation fonctionne, il faut s'assurer qu'on a un operator= de move pour ListeActeurs.
    cout << "Création Film " << film->titre_ << endl;

    for ([[maybe_unused]] auto i : range(nActeurs)) {  // On peut aussi mettre nElements avant et faire un span, comme on le faisait au TD précédent.
        film->acteurs_.Liste<Acteur>::ajouter(lireActeur(fichier, bibliotheque));
    }

    return film;
}

unique_ptr<Livre> lireLivre(const string& fichier) {
    unique_ptr<Livre> livre = make_unique<Livre>();

    vector<string> vectorStrings;
    stringstream ss(fichier);
    string tmp;
    while (getline(ss, tmp, '\t'))
    {
        tmp.erase(remove(tmp.begin(), tmp.end(), '\"'), tmp.end());
        vectorStrings.push_back(tmp);
    }

    livre->titre_ = vectorStrings[0];
    livre->annee_ = stoi(vectorStrings[1]);
    livre->auteur_ = vectorStrings[2];
    livre->millionsCopiesVendues_ = stoi(vectorStrings[3]);
    livre->nombresPages_ = stoi(vectorStrings[4]);
    cout << "Création Livre " << livre->titre_ << endl;

    return livre;

}

const string& Item::obtenirTitre() const { return titre_; };


ostream& operator<< (ostream& os, const Acteur& acteur)
{
    return os << "  " << acteur.nom_ << ", " << acteur.anneeNaissance_ << " " << acteur.sexe_ << endl;
}

ostream& operator<< (ostream& os, const Affichable& affichable)
{
    affichable.afficher(os);
    return os;
}


void Item::afficher(ostream& os) const
{

    os << titre_;

}

void Film::afficher(ostream& os) const
{
    Item::afficher(os);
    os << ", par " << realisateur_;


}

void Livre::afficher(ostream& os) const
{
    Item::afficher(os);
    os << ", de " <<  auteur_;
}

void FilmLivre::afficher(ostream& os) const
{
    os << FilmLivre::titre_ << ", par ";
    os << FilmLivre::realisateur_ << " , de ";
    os << FilmLivre::auteur_;
}

ostream& operator<<(ostream& os, const Film& film) {
    film.afficher(os);
    return os;
}

ostream& operator<<(ostream& os, const Livre& livre) {
    livre.afficher(os);
    return os;
}

ostream& operator<<(ostream& os, const FilmLivre& filmLivre) {
    filmLivre.afficher(os);
    return os;
}


Bibliotheque::Bibliotheque(string nomFichierFilm, string nomFichierLivre) {
    ifstream fichierFilm(nomFichierFilm, ios::binary);
    fichierFilm.exceptions(ios::failbit);
    int nElements = int(lireUintTailleVariable(fichierFilm));

    for ([[maybe_unused]] int i : range(nElements)) {
        ajouterItem(lireFilm(fichierFilm, *this));
    }

    ifstream fichierLivre(nomFichierLivre);
    if (fichierLivre.fail()) {
        cout << "Erreur lors de l'ouverture du fichier " << nomFichierLivre << endl;
    }
    string ligne;
    while (getline(fichierLivre, ligne)) {
        ajouterItem(lireLivre(ligne));
    }
}

ostream& operator<<(ostream& os, const Bibliotheque& bibliotheque) {
    os << "Bibliothèque: " << endl;
    for (auto&& item : span(bibliotheque.items_)) {
        if (item) {  // Vérifie si l'élément est non nul
            item->afficher(os);
            os << '\n';
        }
    }

    return os;
}

template <typename T>
using EnSpanConst = span<const ranges::range_value_t<T>>;

template <typename T>
concept ConvertibleEnSpanConst = is_convertible_v<T, EnSpanConst<T>>;

template <typename T>
requires ranges::input_range<T> && (!ConvertibleEnSpanConst<T>)
ostream& afficherListeItems(ostream& os, const T& listeItems){
    for (auto&& item : listeItems) {
        if (item) {  // Vérifie si l'élément est non nul
            item->afficher(os);
            os << '\n';
        }
    }
    return os;
}

template <ConvertibleEnSpanConst T>
ostream& afficherListeItems(ostream& os, const T& listeItems){
    for (auto&& item : EnSpanConst<T>(listeItems)) {
        if (item) {  // Vérifie si l'élément est non nul
            item->afficher(os);
            os << '\n';
        }
    }
    return os;
}

int main()
{
#ifdef VERIFICATION_ALLOCATION_INCLUS
    bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
#endif
    bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

    static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

    cout << "Création Bibliothèque: \n";
    Bibliotheque bibliotheque("films.bin", "livres.txt");
    cout << ligneDeSeparation;

    cout << "Affichage du premier Film: \n";
    cout << *bibliotheque[0];
    cout << ligneDeSeparation;

    cout << "Affichage du premier Livre: \n";
    cout << *bibliotheque[8];
    cout << ligneDeSeparation;

    cout << "Affichage de la bibliothèque: \n";
    cout << bibliotheque;


    cout << ligneDeSeparation;

    auto hobbitFilm = bibliotheque.trouver([](const Item& item) {
        return dynamic_cast<const Film*>(&item) != nullptr && item.obtenirTitre() == "Le Hobbit : La Bataille des Cinq Armées";
    });

    auto hobbitLivre = bibliotheque.trouver([](const Item& item) {
        return dynamic_cast<const Livre*>(&item) != nullptr && item.obtenirTitre() == "The Hobbit";
    });

    if (hobbitFilm && hobbitLivre) {

        auto filmLivreHobbit = make_unique<FilmLivre>(*dynamic_cast<Film*>(hobbitFilm.get()), *dynamic_cast<Livre*>(hobbitLivre.get()));
        bibliotheque.ajouterItem(std::move(filmLivreHobbit));
    }
    else {
        cout << "Film 'Le Hobbit' ou livre correspondant non trouvé." << endl;
    }

    cout << "Affichage de la bibliothèque après ajout du combo 'Le Hobbit': \n";

    cout << bibliotheque;
    cout << ligneDeSeparation;

    //0.
    cout << "AfficherListeItems avec vector: \n";
    afficherListeItems(cout, bibliotheque.obtenirItems());
    cout << ligneDeSeparation;

    cout << "AfficherListeItems avec list: \n";
    list<unique_ptr<Item>> listBibli;

    auto debut = bibliotheque.obtenirItems().begin();
    auto fin = bibliotheque.obtenirItems().end();
    for (auto it = debut; it != fin; ++it) {
        const auto& item = *it;
        if (item) { // Vérifie si l'élément est non nul
            listBibli.push_back(unique_ptr<Item>(dynamic_cast<Item*>(item->copier().release())));
        }

    }
    afficherListeItems(cout, listBibli);
    cout << ligneDeSeparation;

    //1.
    //1.1
    cout << "AfficherListeItems avec forward_list: \n";
    forward_list<Item*> forwardListBibli;
    debut = bibliotheque.obtenirItems().begin();
    fin = bibliotheque.obtenirItems().end();

    auto itList = forwardListBibli.before_begin();
    for (auto it = debut; it != fin; ++it) {
        const auto &item = *it;
        if (item) {// Vérifie si l'élément est non nul
            forwardListBibli.insert_after(itList, item.get());
            ++itList;
        }
    }
    afficherListeItems(cout, forwardListBibli);
    cout << ligneDeSeparation;

    //1.2
    cout << "Forward_list à l'envers: \n";
    forward_list<Item*> forwardListBibliInverse;
    auto debutForwardList = forwardListBibli.begin();
    auto finForwardList = forwardListBibli.end();

    for (auto it = debutForwardList; it != finForwardList; ++it) {
        const auto &item = *it;
        if (item) {// Vérifie si l'élément est non nul
            forwardListBibliInverse.push_front(item);
        }
    }

    afficherListeItems(cout, forwardListBibliInverse);
    cout << ligneDeSeparation;

    //1.3
    cout << "Forward_list inversé copier dans une autre forward_list: \n";
    forward_list<Item*> forwardListBibliCopie;
    forwardListBibliInverse.reverse();
    debutForwardList = forwardListBibli.begin();
    finForwardList = forwardListBibli.end();

    itList = forwardListBibliCopie.before_begin();
    for (auto it = debutForwardList; it != finForwardList; ++it) {
        const auto &item = *it;
        if (item) {// Vérifie si l'élément est non nul
            forwardListBibli.insert_after(itList, item);
            ++itList;
        }
    }

    afficherListeItems(cout, forwardListBibliCopie);
    cout << ligneDeSeparation;


    //1.4
    cout << "Forward_list original copier dans un vector à l'enevers: \n";
    int index = distance(forwardListBibli.begin(), forwardListBibli.end()); // la fonction distance est O(n)
    vector<Item*> vectorBibli(index); // instancier un vector est O(n)

    debutForwardList = forwardListBibli.begin(); // une assignation est 0(1)
    finForwardList = forwardListBibli.end(); // une assignation est 0(1)

    for (auto it = debutForwardList; it != finForwardList; ++it) { // une assignation, une comparaison et une addition dans une loop for sont O(n) : (1 + 1 + 1) * n = 3n
        const auto &item = *it;
        if (item) { // un comparaison est dans un boucle for est 0(n) : 1 * n = n
            vectorBibli[--index] = item; // l'opération [] dans une boucle for est 0(n) : 1 * n = n
        }
    }
    // La boucle for est O(n) : 3n + n + n = 5n la complexité maximal dans la boucle est O(n)

    // L'algorithme est O(n) : n + n + n + 2 = 3n + 2 la complexité maximal dans l'algorithme est O(n)

    afficherListeItems(cout, vectorBibli);
    cout << ligneDeSeparation;

    //1.5
    cout << "Acteurs du premier film Alien: \n";
    Film*  film = dynamic_cast<Film*>(bibliotheque[0].get());
    for(auto&& acteur : film->obtenirActeurs()){
        cout << *acteur;
    }

    cout << ligneDeSeparation;




    return 0;


}
