// This file is part of Hermes2D.
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __H2D_MESH_FN_H
#define __H2D_MESH_FN_H

#include "function.h"
#include "../mesh/refmap.h"
#include "../mesh/mesh.h"

namespace Hermes
{
  namespace Hermes2D
  {
    class PrecalcShapeset;
    /// \brief Represents a function defined on a mesh.
    ///
    /// MeshFunction is a base class for all classes representing an arbitrary function
    /// superimposed on a mesh (ie., domain). These include the Solution, ExactSolution
    /// and Filter classes, which define the concrete behavior and the way the function
    /// is (pre)calculated. Any such function can later be visualized.
    ///
    /// (This is an abstract class and cannot be instantiated.)
    ///
    template<typename Scalar>
    class HERMES_API MeshFunction : public Function<Scalar>
    {
    public:
      MeshFunction();
      MeshFunction(Mesh *mesh);
      virtual ~MeshFunction() = 0;

      virtual void init() {};
      virtual void reinit() { this->free(); init();};

      void set_quad_2d(Quad2D* quad_2d)
      {
        Function<Scalar>::set_quad_2d(quad_2d);
        refmap->set_quad_2d(quad_2d);
      }

      virtual void set_active_element(Element* e);

      Mesh*   get_mesh() const { return mesh; }
      RefMap* get_refmap() { this->update_refmap(); return refmap; }

      virtual int get_edge_fn_order(int edge);

      virtual Scalar get_pt_value(double x, double y, int item = H2D_FN_VAL_0) = 0;

      /// Handling overflows. Has to be virtual, because
      /// the necessary iterators in the templated class do not work with GCC.
      virtual void handle_overflow_idx()
      {
        if(this->overflow_nodes != NULL) 
        {
          for(unsigned int i = 0; i < this->overflow_nodes->get_size(); i++)
            if(this->overflow_nodes->present(i))
              ::free(this->overflow_nodes->get(i));
          delete this->overflow_nodes;
        }
        this->nodes = new LightArray<Function<Scalar>::Node *>;
        this->overflow_nodes = this->nodes;
      }

      /// See Transformable::push_transform.
      void push_transform(int son)
      {
        Transformable::push_transform(son);
        this->update_nodes_ptr();
      }

      void pop_transform()
      {
        Transformable::pop_transform();
        this->update_nodes_ptr();
      }

    protected:

      int mode;
      Mesh* mesh;
      RefMap* refmap;

    public:

      /// For internal use only.
      void force_transform(MeshFunction<Scalar>* mf)
      { 
        Function<Scalar>::force_transform(mf->get_transform(), mf->get_ctm()); 
      }

      void update_refmap()
      { refmap->force_transform(this->sub_idx, this->ctm); }

      void force_transform(uint64_t sub_idx, Trf* ctm)
      {
        this->sub_idx = sub_idx;
        this->ctm = ctm;
      }
    };
  }
}
#endif
