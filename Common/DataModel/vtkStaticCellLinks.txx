/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLinks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStaticCellLinks.h"

#ifndef vtkStaticCellLinks_txx
#define vtkStaticCellLinks_txx

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"


//----------------------------------------------------------------------------
// Build the link list array for unstructured grids
template <typename TIds> void vtkStaticCellLinks<TIds>::
BuildLinks(vtkUnstructuredGrid *ugrid)
{
  // Basic information about the grid
  this->NumCells = ugrid->GetNumberOfCells();
  this->NumPts = ugrid->GetNumberOfPoints();

  // We're going to get into the guts of the class
  vtkCellArray *cellArray = ugrid->GetCells();
  const vtkIdType *cells = cellArray->GetPointer();

  // I love this trick: the size of the Links array is equal to
  // the size of the cell array, minus the number of cells.
  this->LinksSize =
    cellArray->GetNumberOfConnectivityEntries() - this->NumCells;

  // Extra one allocated to simplify later pointer manipulation
  this->Links = new TIds[this->LinksSize+1];
  this->Links[this->LinksSize] = this->NumPts;
  this->Offsets = new TIds[this->NumPts+1];
  std::fill_n(this->Offsets, this->NumPts, 0);

  // Now create the links.
  vtkIdType npts, cellId, ptId;
  const vtkIdType *cell=cells;
  int i;

  // Count number of point uses
  for ( cellId=0; cellId < this->NumCells; ++cellId )
    {
    npts = *cell++;
    for (i=0; i<npts; ++i)
      {
      this->Offsets[*cell++]++;
      }
    }

  // Perform prefix sum
  for ( ptId=0; ptId < this->NumPts; ++ptId )
    {
    npts = this->Offsets[ptId+1];
    this->Offsets[ptId+1] = this->Offsets[ptId] + npts;
    }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for ( cell=cells, cellId=0; cellId < this->NumCells; ++cellId )
    {
    npts = *cell++;
    for (i=0; i<npts; ++i)
      {
      this->Offsets[*cell]--;
      this->Links[this->Offsets[*cell++]] = cellId;
      }
    }
  this->Offsets[this->NumPts] = this->LinksSize;
}

//----------------------------------------------------------------------------
// Build the link list array for poly data. This is more complex because there
// are potentially fout different cell arrays to contend with.
template <typename TIds> void vtkStaticCellLinks<TIds>::
BuildLinks(vtkPolyData *pd)
{
  // Basic information about the grid
  this->NumCells = pd->GetNumberOfCells();
  this->NumPts = pd->GetNumberOfPoints();

  const vtkCellArray *cellArrays[4];
  vtkIdType numCells[4];
  vtkIdType sizes[4];

  cellArrays[0] = pd->GetVerts();
  cellArrays[1] = pd->GetLines();
  cellArrays[2] = pd->GetPolys();
  cellArrays[3] = pd->GetStrips();

  for (i=0; i<4; ++i)
    {
    if ( cellArrays[i] != NULL )
      {
      numCells[i] = cellArrays[i]->GetNumberOfCells();
      sizes[i] = cellArrays[i]->GetNumberOfConnectivityEntries() - numCells[i];
      }
    else
      {
      numCells[i] = 0;
      sizes[i] = 0;
      }
    }//for the four polydata arrays

  // Allocate
  this->LinksSize = sizes[0] + sizes[1] + sizes[2] + sizes[3];
  this->Links = new TIds[this->LinksSize];
  this->Links[this->LinksSize] = this->NumPts;
  this->Offsets = new TIds[this->NumPts+1];
  this->Offsets[this->NumPts] = this->LinksSize;
  std::fill_n(this->Offsets, this->NumPts, 0);

  // Now create the links.
  vtkIdType npts, cellId, CellId, ptId;
  const vtkIdType *cell;
  int i, j;

  // Visit the four arrays
  for ( CellId=0, j=0; j < 4; ++j )
    {
    // Count number of point uses
    cell = cellArrays[j];
    for ( cellId=0; cellId < numCells[i]; ++cellId )
      {
      npts = *cell++;
      for (i=0; i<npts; ++i)
        {
        this->Offsets[CellId+(*cell)++]++;
        }
      }
    CellId += numCells[j];
    } //for each of the four polydata cell arrays

  // Perform prefix sum
  for ( ptId=0; ptId < this->NumPts; ++ptId )
    {
    npts = this->Offsets[ptId+1];
    this->Offsets[ptId+1] = this->Offsets[ptId] + npts;
    }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for ( CellId=0, j=0; j < 4; ++j )
    {
    cell = cellArrays[j];
    for ( cellId=0; cellId < numCells[i]; ++cellId )
      {
      npts = *cell++;
      for (i=0; i<npts; ++i)
        {
        this->Offsets[*cell]--;
        this->Links[this->Offsets[*cell++]] = CellId+cellId;
        }
      }
    CellId += numCells[j];
    }//for each of the four polydata arrays
}

#endif
