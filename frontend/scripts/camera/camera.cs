using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework;
using Objects;
using System.Data;
using System.Reflection.Metadata.Ecma335;
using System.Data.Common;

namespace Cameras;

public abstract class Camera
{
    public Vector3 position { 
        get { return _position; } 
        set { 
            _position = value; 
            view_matrix = Matrix.Invert(Matrix.CreateFromQuaternion(_rotation) * Matrix.CreateTranslation(value)); 
        } 
    }
    private Vector3 _position;

    public Quaternion rotation { 
        get { return _rotation; } 
        set {
            _rotation = value; 
            view_matrix = Matrix.Invert(Matrix.CreateFromQuaternion(value) * Matrix.CreateTranslation(_position)); 
        }
    }
    public Quaternion _rotation;

    public float field_of_view { get{ return _field_of_view; } set { _field_of_view = value; this.projection_matrix = Matrix.CreatePerspectiveFieldOfView(_field_of_view, _aspect_ratio, 0.01f, 1000f); } }
    private float _field_of_view;

    public float aspect_ratio { get{ return _aspect_ratio; } set { _aspect_ratio = value; this.projection_matrix = Matrix.CreatePerspectiveFieldOfView(_field_of_view, _aspect_ratio, 0.01f, 1000f); } }
    private float _aspect_ratio;

    public Matrix view_matrix;
    public Matrix projection_matrix;

    public Camera(Vector3 position, Quaternion rotation, float field_of_view, float aspect_ratio)
    {
        this._position = position;
        this._rotation = rotation;
        this._field_of_view = field_of_view;
        this._aspect_ratio = aspect_ratio;

        this.view_matrix = Matrix.Invert(Matrix.CreateFromQuaternion(rotation) * Matrix.CreateTranslation(position));

        this.projection_matrix = Matrix.CreatePerspectiveFieldOfView(field_of_view, aspect_ratio, 0.01f, 1000f);
    }

    public abstract void Update(float dt);

    public virtual void Move(Vector3 change)
    {
        this.position += Vector3.Transform(change, _rotation);
    }

    public virtual void Rotate(Quaternion rotation)
    {
        this.rotation *= rotation;
    }
}

public class FollowCamera : Camera
{
    public Object object_to_follow;

    public Vector3 object_offset;
    private Vector3 move_to_position;

    public FollowCamera(Vector3 offset, Object object_to_follow) : base(Vector3.Zero, Quaternion.Identity, 2.0f, 1.0f)
    {
        this.object_offset = offset;
        this.object_to_follow = object_to_follow;

        this.move_to_position = object_to_follow.position + offset;
    }

    public override void Update(float dt)
    {
        this.move_to_position = this.object_to_follow.position + this.object_offset;
        this.position = Vector3.Lerp(this.position, this.move_to_position, dt);
    }
}

public class SimpleCamera : Camera
{
    /// <summary>
    /// the radians rotation of the camera around the x axis
    /// </summary>
    public float x_rotation { get{ return _x_rotation; } set { _x_rotation = value; }}
    private float _x_rotation = 0;

    /// <summary>
    /// the radians rotation of the camera around the y axis
    /// </summary>
    public float y_rotation { get{ return _y_rotation; } set { _y_rotation = value; }}
    private float _y_rotation = 0;

    public SimpleCamera(Vector3 position) : base(position, Quaternion.Identity, 2.0f, 1.0f)
    {

    }

    /// <summary>
    /// Rotate the camera around the x and y axii by the repsective delta values
    /// </summary>
    /// <param name="d_yaw"> The amount of radians to rotate the camera around the y axis, the yaw </param>
    /// <param name="d_pitch"> The amount of radians to rotate the camera around the x axis, the pitch </param>
    public void Rotate(float d_yaw, float d_pitch)
    {
        if(d_yaw == 0 && d_pitch == 0) { return; }

        this.x_rotation += d_yaw;
        this.y_rotation += d_pitch;

        this.rotation = Quaternion.CreateFromYawPitchRoll(x_rotation, y_rotation, 0f);
    }

    public override void Update(float dt)
    {
        throw new System.NotImplementedException();
    }
}