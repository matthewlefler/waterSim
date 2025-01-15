using System.Linq;
using System.Runtime.CompilerServices;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public class Simulation
{
    public Vector4[] cells; // x, y, z   y+ is up
    public VoxelObject voxelObject;

    public int width;
    public int height;
    public int depth;

    public Simulation()
    {
        this.cells = new Vector4[0];

        this.width = 0;
        this.height = 0;
        this.depth = 0;


        Color[] colors = new Color[this.cells.Length];
        Vector3[] positions = new Vector3[this.cells.Length];

        for (int i = 0; i < this.cells.Length; i++)
        {
            colors[i] = new Color(this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.1f);
    }

    public Simulation(Vector4[] cells, int width, int height, int depth)
    {
        this.cells = cells;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.cells.Length];
        Vector3[] positions = new Vector3[this.cells.Length];

        for (int i = 0; i < this.cells.Length; i++)
        {
            colors[i] = new Color(this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject = new VoxelObject(Vector3.Zero, Quaternion.Identity, positions, colors, 0.1f);
    }

    public void SetData(Vector4[] new_cells, int width, int height, int depth)
    {
        this.cells = new_cells;

        this.width = width;
        this.height = height;
        this.depth = depth;

        Color[] colors = new Color[this.cells.Length];
        Vector3[] positions = new Vector3[this.cells.Length];

        for (int i = 0; i < this.cells.Length; i++)
        {
            colors[i] = new Color(this.cells[i].W, this.cells[i].W, this.cells[i].W);
            positions[i] = this.Get3DPositionVec(i);
        }

        this.voxelObject.SetData(positions, colors);
    } 

    public (int x, int y, int z) Get3DPosition(int index)
    {
        return (index % width, (index / height) % (width * height), index % (width * height));
    }  

    public Vector3 Get3DPositionVec(int index)
    {
        return new Vector3(index % width, (index / height) % (width * height), index % (width * height));
    }  

    public int GetIndex(int x, int y, int z)
    {
        return x + (y * height) + (z * width * height);
    } 
}