using System.Diagnostics;
using System.Reflection;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;

namespace Objects;

public abstract class Object
{
    public Vector3 position;
    public Quaternion rotation;

    public Object(Vector3 position, Quaternion rotation)
    {
        this.position = position;
        this.rotation = rotation;
    }
}

public class VoxelObject : Object
{
    public Vector3[] voxel_cube_offsets;
    public float size;

    public Color[] voxel_colors; // colors of the voxels
    public VertexPositionColor[] vertices;

    public VoxelObject(Vector3 position, Quaternion rotation, Vector3[] voxel_positions, Color[] voxel_colors, float voxel_size) : base(position, rotation)
    {
        Debug.Assert(voxel_colors.Length == voxel_positions.Length);

        this.voxel_cube_offsets = voxel_positions;
        vertices = new VertexPositionColor[voxel_cube_offsets.Length * 12 * 3]; // 12 triangles * 3 vertex per triangle 
        this.voxel_colors = voxel_colors;
        this.size = voxel_size;

        UpdateVertices();
    }

    public void SetData(Vector3[] voxel_positions, Color[] voxel_colors)
    {
        this.voxel_cube_offsets = voxel_positions;
        this.voxel_colors = voxel_colors;
    }

    public void UpdateVertices()
    {
        vertices = new VertexPositionColor[voxel_cube_offsets.Length * 12 * 3]; // 12 triangles * 3 vertex per triangle 

        Vector3[] cube_points = new Vector3[] {
            (Vector3.Up   + Vector3.Forward  + Vector3.Left ) * size, // vertex 0
            (Vector3.Up   + Vector3.Forward  + Vector3.Right) * size, // vertex 1
            (Vector3.Up   + Vector3.Backward + Vector3.Left ) * size, // vertex 2
            (Vector3.Up   + Vector3.Backward + Vector3.Right) * size, // vertex 3
            (Vector3.Down + Vector3.Forward  + Vector3.Left ) * size, // vertex 4
            (Vector3.Down + Vector3.Forward  + Vector3.Right) * size, // vertex 5
            (Vector3.Down + Vector3.Backward + Vector3.Left ) * size, // vertex 6
            (Vector3.Down + Vector3.Backward + Vector3.Right) * size, // vertex 7
        };

        /*
         *     0 ______1
         *     /|     /|
         *    / |    / |
         *  2/__|___/3 |
         *   |  |__|___|
         *   | 4/  |  /5
         *   | /   | /
         *   |/____|/
         *   6      7
        */

        for(int i = 0, j = 0; i < this.voxel_cube_offsets.Length; i++, j+=36)
        {
            Vector3 voxel_offset = voxel_cube_offsets[i];

            vertices[j   ] = new VertexPositionColor(voxel_offset + cube_points[0], voxel_colors[i]); // triangle 1 TOP FACE
            vertices[j+1 ] = new VertexPositionColor(voxel_offset + cube_points[1], voxel_colors[i]);
            vertices[j+2 ] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]);

            vertices[j+3 ] = new VertexPositionColor(voxel_offset + cube_points[2], voxel_colors[i]); // triangle 2
            vertices[j+4 ] = new VertexPositionColor(voxel_offset + cube_points[0], voxel_colors[i]);
            vertices[j+5 ] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]);

            vertices[j+6 ] = new VertexPositionColor(voxel_offset + cube_points[2], voxel_colors[i]); // triangle 3 FRONT FACE
            vertices[j+7 ] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]);
            vertices[j+8 ] = new VertexPositionColor(voxel_offset + cube_points[6], voxel_colors[i]);

            vertices[j+9 ] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]); // triangle 4
            vertices[j+10] = new VertexPositionColor(voxel_offset + cube_points[7], voxel_colors[i]);
            vertices[j+11] = new VertexPositionColor(voxel_offset + cube_points[6], voxel_colors[i]);

            vertices[j+12] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]); // triangle 5 LEFT FACE
            vertices[j+13] = new VertexPositionColor(voxel_offset + cube_points[0], voxel_colors[i]);
            vertices[j+14] = new VertexPositionColor(voxel_offset + cube_points[2], voxel_colors[i]);

            vertices[j+15] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]); // triangle 6
            vertices[j+16] = new VertexPositionColor(voxel_offset + cube_points[2], voxel_colors[i]);
            vertices[j+17] = new VertexPositionColor(voxel_offset + cube_points[6], voxel_colors[i]);

            vertices[j+18] = new VertexPositionColor(voxel_offset + cube_points[7], voxel_colors[i]); // triangle 7 RIGHT FACE
            vertices[j+19] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]);
            vertices[j+20] = new VertexPositionColor(voxel_offset + cube_points[5], voxel_colors[i]);

            vertices[j+21] = new VertexPositionColor(voxel_offset + cube_points[5], voxel_colors[i]); // triangle 8
            vertices[j+22] = new VertexPositionColor(voxel_offset + cube_points[3], voxel_colors[i]);
            vertices[j+23] = new VertexPositionColor(voxel_offset + cube_points[1], voxel_colors[i]);

            vertices[j+24] = new VertexPositionColor(voxel_offset + cube_points[5], voxel_colors[i]); // triangle 9 BACK FACE
            vertices[j+25] = new VertexPositionColor(voxel_offset + cube_points[1], voxel_colors[i]);
            vertices[j+26] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]);

            vertices[j+27] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]); // triangle 10
            vertices[j+28] = new VertexPositionColor(voxel_offset + cube_points[1], voxel_colors[i]);
            vertices[j+29] = new VertexPositionColor(voxel_offset + cube_points[0], voxel_colors[i]);

            vertices[j+30] = new VertexPositionColor(voxel_offset + cube_points[6], voxel_colors[i]); // triangle 11 BOTTOM FACE
            vertices[j+31] = new VertexPositionColor(voxel_offset + cube_points[7], voxel_colors[i]);
            vertices[j+32] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]);

            vertices[j+33] = new VertexPositionColor(voxel_offset + cube_points[4], voxel_colors[i]); // triangle 12
            vertices[j+34] = new VertexPositionColor(voxel_offset + cube_points[7], voxel_colors[i]);
            vertices[j+35] = new VertexPositionColor(voxel_offset + cube_points[5], voxel_colors[i]);
        }
    }
}

#nullable enable