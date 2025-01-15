using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework;
using Objects;
using System.Data;
using System.Reflection.Metadata.Ecma335;

namespace Cameras;

public abstract class Camera
{
    public Vector3 position { 
        get { return _position; } 
        set { _position = value; view_matrix = Matrix.CreateTranslation(value) * Matrix.CreateFromQuaternion(_rotation); } 
    }
    private Vector3 _position;

    public Quaternion rotation { 
        get { return _rotation; } 
        set {_rotation = value; view_matrix = Matrix.CreateTranslation(_position) * Matrix.CreateFromQuaternion(value); }
    }
    public Quaternion _rotation;

    float field_of_view;

    public Matrix view_matrix;
    public Matrix projection_matrix;

    public Camera(Vector3 position, Quaternion rotation, float field_of_view, float aspect_ratio)
    {
        this._position = position;
        this._rotation = rotation;
        this.field_of_view = field_of_view;

        this.view_matrix = Matrix.CreateTranslation(position) * Matrix.CreateFromQuaternion(rotation);

        this.projection_matrix = Matrix.CreatePerspectiveFieldOfView(field_of_view, aspect_ratio, 0.01f, 1000f);
    }

    public abstract void Update(float dt);

    public virtual void Move(Vector3 change)
    {
        this.position += Vector3.Transform(change, _rotation);
    }

    public virtual void Rotate(Quaternion quaternion)
    {
        this.rotation = this._rotation + quaternion;
    }
}

public class FollowCamera : Camera
{
    public Object object_to_follow;

    public Vector3 object_offset;
    private Vector3 move_to_position;

    public FollowCamera(Vector3 offset, Object object_to_follow) : base(Vector3.Zero, Quaternion.Identity, 90.0f, 1.0f)
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
    public SimpleCamera() : base(Vector3.Zero, Quaternion.Identity, 90.0f, 1.0f)
    {

    }

    public override void Update(float dt)
    {
        throw new System.NotImplementedException();
    }
}