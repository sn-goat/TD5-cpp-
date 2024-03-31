/**
* Ficher hpp qui contient la définition des class Liste,
 * bibliothèque, Affichable, Item, Film, Acteur, Livre et FilmLivre.
* \file classes_td5.Hpp
* \author Nyouvop et Haddad
* \date 31 mars 2024
* Créé le 25 mars  2024
*/
#pragma once
// Structures mémoires pour une collection de films.

#include <string>
#include <memory>
#include <functional>
#include <cassert>
#include "gsl/span"
using gsl::span;
using namespace std;

class Acteur; class Item; class Affichable; class Film; class Livre;

template <typename T>
class Liste {
public:
    Liste() = default;
    explicit Liste(int capaciteInitiale) :
        capacite_(capaciteInitiale),
        elements_(capaciteInitiale)
    {
    }
    Liste(const Liste<T>& autre) :
        capacite_(autre.nElements_),
        nElements_(autre.nElements_),
        elements_(autre.nElements_)
    {
        for (int i = 0; i < nElements_; ++i)
            elements_[i] = autre.elements_[i];
    }

    Liste(Liste<T>&&) = default;
    Liste<T>& operator= (Liste<T>&&) noexcept = default;

    void ajouter(shared_ptr<T> element)
    {
        assert(nElements_ < capacite_);
        elements_[nElements_++] = std::move(element);
    }

    shared_ptr<T>& operator[] (int index) const { assert(0 <= index && index < nElements_); return elements_[index]; }
    span<shared_ptr<T>> enSpan() const { return span(elements_); }

    const vector<shared_ptr<T>>& obtenirElements() const { return elements_; }




private:
    int capacite_ = 0, nElements_ = 0;
    vector<shared_ptr<T>> elements_;
};

using ListeActeurs = Liste<Acteur>;



class Bibliotheque {
public:
    Bibliotheque() = default;
    Bibliotheque(string nomFichierFilm, string nomFichierLivre);
    void ajouterItem(unique_ptr<Item> item);
    void enleverItem(const unique_ptr<Item>& item);
    shared_ptr<Acteur> trouverActeur(const string& nomActeur) const;
    span<unique_ptr<Item>> obtenirItems();
    [[nodiscard]] int size() const { return items_.size(); }
    unique_ptr<Item>& operator[] (int index) { assert(0 <= index && index < size());  return items_[index]; }

    template<typename T>
    unique_ptr<Item> trouver(const T& critere) {
        for (auto&& Item : span(items_))
            if (critere(*Item))
                return std::move(Item);
        return nullptr;
    }
    friend ostream& operator<<(ostream& os, const Bibliotheque& bibliotheque);



private:
    vector<unique_ptr<Item>> items_;

};

class Affichable {
public:
    virtual void afficher(ostream& os) const = 0;
    friend ostream& operator<< (ostream& os, const Affichable& affichable);
    virtual ~Affichable() = default;
};

class Copiable {
public:
    virtual unique_ptr<Copiable> copier() = 0;
    virtual ~Copiable() = default;

};

class Item : public Affichable, public Copiable {
public:
    void afficher(ostream& os) const override;
    Item() = default;
    Item(const Item&) = delete;
    Item& operator= (const Item&) = delete;
    virtual ~Item() = default;

    const string& obtenirTitre() const;
    virtual const vector<shared_ptr<Acteur>>& obtenirActeurs() const {
        static const vector<shared_ptr<Acteur>>  vecteur(0);
        return vecteur;
    };

    friend unique_ptr<Film> lireFilm(istream& fichier, Bibliotheque& bibliotheque);
    friend unique_ptr<Livre> lireLivre(const string& fichier);

    unique_ptr<Copiable> copier() override { return make_unique<Item>(CopySlice, *this); }

private:
    int annee_ = 0;


protected:
    struct CopySlice_t { explicit CopySlice_t() = default; };
    static constexpr CopySlice_t CopySlice{};
    string titre_;

public:
    Item(CopySlice_t, const Item& item) : annee_(item.annee_), titre_(item.titre_) {};
};


class Film :virtual public Item {
public:
    Film() = default;
    void afficher(ostream& os) const override;
    friend ostream& operator<<(ostream& os, const Film& film);

    friend unique_ptr<Film> lireFilm(istream& fichier, Bibliotheque& Bibliotheque);

    const vector<shared_ptr<Acteur>>& obtenirActeurs() const override;

    unique_ptr<Copiable> copier() override { return make_unique<Film>(CopySlice, *this); }
    int obtenirRecette() const { return recette_; }
private:
    int recette_ = 0;
    ListeActeurs acteurs_;

protected:
    string realisateur_;

public:
    Film(CopySlice_t, const Film& film) : Item(CopySlice, film), recette_(film.recette_),
        acteurs_(film.acteurs_), realisateur_(film.realisateur_) {}
};

class Livre : virtual public Item {
public:
    Livre() = default;
    void afficher(ostream& os) const override;
    friend ostream& operator<<(ostream& os, const Livre& livre);
    friend unique_ptr<Livre> lireLivre(const string& fichier);
    unique_ptr<Copiable> copier() override { return make_unique<Livre>(CopySlice, *this); }


private:
    int millionsCopiesVendues_, nombresPages_;

protected:
    string  auteur_;

public:
    Livre(CopySlice_t, const Livre& livre) : Item(CopySlice, livre), millionsCopiesVendues_(livre.millionsCopiesVendues_),
        nombresPages_(livre.nombresPages_), auteur_(livre.auteur_) {}
};

class FilmLivre : public Film, public Livre {
public:
    FilmLivre(const Film& film, const Livre& livre) : Item(CopySlice, film), Film(CopySlice, film),
        Livre(CopySlice, livre) {}

    FilmLivre(CopySlice_t, const FilmLivre& filmLivre) : Item(CopySlice, filmLivre), Film(CopySlice, filmLivre),
        Livre(CopySlice, filmLivre) {}

    void afficher(ostream& os) const override;
    friend ostream& operator<<(ostream& os, const FilmLivre& filmLivre);

    unique_ptr<Copiable> copier() override { return make_unique<FilmLivre>(CopySlice, *this); }

};

class Acteur {
public:
    friend shared_ptr<Acteur> lireActeur(istream& fichier, const Bibliotheque& bibliotheque);

    const string& obtenirNom() const { return nom_; }

    friend ostream& operator<< (ostream& os, const Acteur& acteur);

private:
    string nom_; int anneeNaissance_ = 0; char sexe_ = '\0';
};
