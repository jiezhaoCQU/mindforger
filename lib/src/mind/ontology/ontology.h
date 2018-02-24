/*
 ontology.h     MindForger thinking notebook

 Copyright (C) 2016-2018 Martin Dvorak <martin.dvorak@mindforger.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef M8R_ONTOLOGY_H_
#define M8R_ONTOLOGY_H_

// IMPROVE something is wrong w/ this header - had to include it in .cpp files in AST

#include <string>
#include <set>

#include "thing.h"
#include "clazz.h"
#include "taxonomy.h"
#include "relationship_type.h"

#include "../../config/configuration.h"
#include "../../model/tag.h"
#include "../../model/outline_type.h"
#include "../../model/note_type.h"
#include "ontology_vocabulary.h"

namespace m8r {

class Configuration;

/**
 * @brief Mind ontology.
 *
 * This class is a rationalization of the comprehensive academic ontology theory to
 * a reasonable extend that suits MindForger needs.
 *
 * An ontology is a formal naming and definition of the types, properties,
 * and interrelationships of the entities that exist in a particular domain.
 * Ontologies are created to limit complexity and to organize information in
 * Memory for Mind.
 *   Ontology includes classes for entities, taxonomies (ISA hiearchy of classes),
 * relationship types (relationship classification), relationships (class 2 class,
 * relationship type 2 relationship type, entity to entity).
 *   Entity is an (C++) instance of anything in the ontology.
 *
 * m8r::Thing is anything in m8r::Ontology i.e. Class, entity (C++ instance),
 * relationship, ... is a Thing. Therefore anything in m8r::Ontology inherits
 * from m8r::Thing.
 *   Also the root of Class hierachy (entity classification) is (C++ instance of)
 * m8r::Class named "Thing".
 *
 * m8r::Clazz is used to define semantical structure (big tree) of Things in Memory (knowledge).
 *
 * m8r::Taxonomy is a m8r::Clazz which is root of a taxonomic tree. Taxonomic tree is a tree
 * of Classes that define a type hierarchy using IS-A relationship.
 *   There are various taxonomies for different (semantical) domains i.e. Class tree is multi-layered
 * with sub-trees that beginning just below Thing and are Taxonomies. Such layer (taxonomy) defines
 * facet i.e. technical, functional, emotional, etc. view of the entity (assigned to entity
 * via m8r::Tag, which is a m8r::Class).
 *
 * m8r::RelationshipType is a m8r::Clazz which can be used to classify (C++) m8r::Relationship
 * (instances).
 *
 * m8r::Relationship is oriented relation between any m8r::Things having a m8r::RelationshipType.
 *   Relationships are used through MindForger model (Outline, Note, ...) to keep explicitly
 * defined relationships.
 *
 * m8r::Triple is binary oriented relationship linking subject entity to object entity via
 * predicate (m8r::RelationshipType).
 *   MindForger by default manages only *explicitly* created relationships among entities
 * within model.
 *   Once MindForger is switched to *thinking* mode, m8r::Mind builds a semantic view Memory of
 * triples i.e. creates triples for explicit rels, implicit (parent/child O/N) rels, inferred
 * rels, ...
 *
 * Binding of the MindForger model to m8r::Ontology is designed as follows.
 *
 * m8r::Tag (a user created C++ instance) is in fact a new m8r::Clazz allowing to classify
 * Memory entities and map them to Taxonomy(ies). Similarly for other types.
 *
 * m8r::Relationship in model is mapped to Mind's m8r::Triple.
 */
class Ontology
{
private:
    // ontology is loaded from the configuration (and by harvesting memory content)
    const Configuration &config;

    /**
     * @brief the root of everything
     */
    Clazz thing;

    /*
     * Taxonomies
     */

    /**
     * @brief Taxonomies.
     *
     * Ontology defined taxonomies like: tags (important, todo, personal, ...),
     * outline types, note types, etc.
     */
    std::set<std::string,Clazz*> taxonomies;

    /**
     * Outline type naming convention: first uppercase; e.g. Grow
     */
    Taxonomy<OutlineType> outlineTypeTaxonomy;

    /**
     * Note type naming convention: first uppercase; e.g. Question
     */
    Taxonomy<NoteType> noteTypeTaxonomy;

    /**
     * @brief Relationship types.
     *
     * Naming convention (is display name): lowercase (:alpha :number space); e.g. ISA, depends on, ...
     */
    Taxonomy<RelationshipType> relationshipTypeTaxonomy;

    /**
     * Tags are shared among Outlines and Notes (Outline can become Note w/ its children
     * and vice versa) - shared tag vocabulary is better than outline labels/note tags/keywords/...
     *
     * Tag naming convention: lowercase (:alpha :number space); e.g. cool, super cool, ...
     */
    Taxonomy<Tag> tagsTaxonomy;

    /*
     * Defaults
     */

    OutlineType* defaultOutlineType;
    NoteType* defaultNoteType;

public:
    Ontology() = delete;
    Ontology(const Configuration &configuration);
    Ontology(const Ontology&) = delete;
    Ontology(const Ontology&&) = delete;
    Ontology &operator=(const Ontology&) = delete;
    Ontology &operator=(const Ontology&&) = delete;
    virtual ~Ontology();

    void load();
    void save() const;

    Clazz* getThing() const { return thing; }

#ifdef DO_MF_DEBUG
    void print() const {}
#endif

    /**
     * @brief Find tag w/ key ensuring it exist in one instance through the application.
     * @return tag w/ given key.
     */
    const Tag* findOrCreateTag(const std::string& key);
    OntologyVocabulary<Tag>& getTags() { return tags; }

    const OutlineType* findOrCreateOutlineType(const std::string& key);
    const OutlineType* getDefaultOutlineType() const { return defaultOutlineType; }
    OntologyVocabulary<OutlineType>& getOutlineTypes() { return outlineTypes; }

    const NoteType* findOrCreateNoteType(const std::string& key);
    const NoteType* getDefaultNoteType() const { return defaultNoteType; }
    OntologyVocabulary<NoteType>& getNoteTypes() { return noteTypes; }
};

} // m8r namespace

#endif /* M8R_ONTOLOGY_H_ */
