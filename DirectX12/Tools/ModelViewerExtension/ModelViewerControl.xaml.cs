using System.IO;
using System.Numerics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Input;
using Microsoft.Win32;

namespace ModelViewerExtension;

public partial class ModelViewerControl : UserControl
{
    private Point m_LastMousePosition;
    private bool m_IsOrbiting;
    private bool m_IsPanning;
    private double m_CameraYaw;
    private double m_CameraPitch = 0.05;
    private double m_CameraDistance = 6.0;
    private Point3D m_CameraTarget = new(0, 1, 0);
    private Rect3D m_ModelBounds = Rect3D.Empty;

    public ModelViewerControl()
    {
        InitializeComponent();
    }

    private void OpenFileClick(object sender, RoutedEventArgs e)
    {
        OpenFileDialog dialog = new()
        {
            Filter = "Runtime model|*.mskn;*.mmat|Skin model|*.mskn|Material|*.mmat|All files|*.*",
            CheckFileExists = true,
        };
        if (dialog.ShowDialog(Window.GetWindow(this)) != true) { return; }
        LoadFile(dialog.FileName);
    }

    public void LoadFile(string filePath)
    {
        try
        {
            if (string.Equals(Path.GetExtension(filePath), ".mmat", StringComparison.OrdinalIgnoreCase)) { ShowMaterial(filePath); }
            else if (string.Equals(Path.GetExtension(filePath), ".mskn", StringComparison.OrdinalIgnoreCase)) { ShowModel(filePath); }
            else { throw new RuntimeFormatException($"Unsupported runtime model extension: {Path.GetExtension(filePath)}"); }
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or RuntimeFormatException)
        {
            ClearModel();
            SetFileHeader(filePath);
            StatusText.Text = exception.Message;
            MessageBox.Show(Window.GetWindow(this), exception.Message, "Runtime Model Viewer", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void ShowMaterial(string filePath)
    {
        RuntimeMaterial material = RuntimeFormatReader.ReadMmat(filePath);
        ClearModel();
        SetFileHeader(filePath);
        SummaryText.Text = "MMAT material\n" + FormatMaterial(material);
        MaterialsList.ItemsSource = new[] { CreateMaterialPanel(material, filePath) };
        StatusText.Text = "MMAT version 2/3 loaded and validated.";
    }

    private void ShowModel(string filePath)
    {
        RuntimeSkinModel model = RuntimeFormatReader.ReadMskn(filePath);
        ClearModel();
        SetFileHeader(filePath);
        SummaryText.Text = $"MSKN v4\n\nVertices       {model.Vertices.Count:N0}\nIndices        {model.Indices.Count:N0}\nBones          {model.Bones.Count:N0}\nSkin slots     {model.SkinSlots.Count:N0}\nSubmeshes      {model.Submeshes.Count:N0}";
        List<FrameworkElement> materialPanels = new();
        for (int i = 0; i < model.Submeshes.Count; ++i)
        {
            RuntimeMaterial? material = model.Materials[i];
            materialPanels.Add(material is null
                ? new TextBlock { Text = $"{i}: {model.Submeshes[i].MaterialPath} (missing)", TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 2, 0, 2) }
                : CreateMaterialPanel(material, model.Submeshes[i].MaterialPath));
        }
        MaterialsList.ItemsSource = materialPanels;
        int resolvedTextureCount = 0;
        ModelVisual.Content = CreateModelGeometry(model, filePath, ref resolvedTextureCount);
        StatusText.Text = $"Loaded and validated. Textures: {resolvedTextureCount}/{model.Materials.Count(material => material is not null)}.";
    }

    private Model3DGroup CreateModelGeometry(RuntimeSkinModel model, string modelFilePath, ref int resolvedTextureCount)
    {
        Point3DCollection positions = new();
        Vector3DCollection normals = new();
        PointCollection textureCoordinates = new();
        foreach (RuntimeSkinVertex vertex in model.Vertices)
        {
            Vector3 position = ApplyBindPose(vertex, model);
            positions.Add(new Point3D(position.X, position.Y, -position.Z));
            normals.Add(new Vector3D(vertex.Normal.X, vertex.Normal.Y, -vertex.Normal.Z));
            textureCoordinates.Add(new System.Windows.Point(vertex.UV.X, vertex.UV.Y));
        }
        Model3DGroup group = new();
        int index_offset = 0;
        foreach (int submesh_index in Enumerable.Range(0, model.Submeshes.Count))
        {
            RuntimeSkinSubmesh submesh = model.Submeshes[submesh_index];
            MeshGeometry3D mesh = new() { Positions = positions, Normals = normals, TextureCoordinates = textureCoordinates };
            for (int index = 0; index < submesh.IndexCount; ++index) { mesh.TriangleIndices.Add(checked((int)model.Indices[index_offset + index])); }
            RuntimeMaterial? runtime_material = model.Materials[submesh_index];
            DiffuseMaterial material;
            if (runtime_material is null)
            {
                material = new DiffuseMaterial(new SolidColorBrush(Color.FromRgb(210, 210, 210)));
            }
            else
            {
                material = CreateDiffuseMaterial(runtime_material, modelFilePath, out bool textureResolved);
                if (textureResolved) { ++resolvedTextureCount; }
            }
            group.Children.Add(new GeometryModel3D(mesh, material) { BackMaterial = material });
            index_offset += checked((int)submesh.IndexCount);
        }
        m_ModelBounds = group.Bounds;
        FrameModel();
        return group;
    }

    private void OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left && e.ClickCount == 2)
        {
            FrameModel();
            e.Handled = true;
            return;
        }
        m_IsOrbiting = e.LeftButton == MouseButtonState.Pressed;
        m_IsPanning = e.RightButton == MouseButtonState.Pressed;
        if (!m_IsOrbiting && !m_IsPanning) { return; }
        m_LastMousePosition = e.GetPosition(PreviewHost);
        PreviewHost.CaptureMouse();
        e.Handled = true;
    }

    private void OnPreviewMouseMove(object sender, MouseEventArgs e)
    {
        if (!m_IsOrbiting && !m_IsPanning) { return; }
        Point position = e.GetPosition(PreviewHost);
        System.Windows.Vector delta = position - m_LastMousePosition;
        m_LastMousePosition = position;

        if (m_IsOrbiting)
        {
            m_CameraYaw -= delta.X * 0.008;
            m_CameraPitch = Math.Max(-1.5, Math.Min(1.5, m_CameraPitch + delta.Y * 0.008));
        }
        if (m_IsPanning)
        {
            PerspectiveCamera camera = (PerspectiveCamera)Viewport.Camera;
            Vector3D forward = camera.LookDirection;
            forward.Normalize();
            Vector3D right = Vector3D.CrossProduct(forward, camera.UpDirection);
            right.Normalize();
            Vector3D up = Vector3D.CrossProduct(right, forward);
            up.Normalize();
            double scale = m_CameraDistance * 0.0018;
            m_CameraTarget += (-right * delta.X + up * delta.Y) * scale;
        }
        UpdateCamera();
        e.Handled = true;
    }

    private void OnPreviewMouseUp(object sender, MouseButtonEventArgs e)
    {
        if (e.ChangedButton == MouseButton.Left) { m_IsOrbiting = false; }
        if (e.ChangedButton == MouseButton.Right) { m_IsPanning = false; }
        if (!m_IsOrbiting && !m_IsPanning) { PreviewHost.ReleaseMouseCapture(); }
        e.Handled = true;
    }

    private void OnPreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        m_CameraDistance = Math.Max(0.01, m_CameraDistance * Math.Exp(-e.Delta * 0.001));
        UpdateCamera();
        e.Handled = true;
    }

    private void FrameModel()
    {
        if (m_ModelBounds.IsEmpty) { return; }
        m_CameraTarget = new Point3D(
            m_ModelBounds.X + m_ModelBounds.SizeX * 0.5,
            m_ModelBounds.Y + m_ModelBounds.SizeY * 0.5,
            m_ModelBounds.Z + m_ModelBounds.SizeZ * 0.5);
        double largestSize = Math.Max(m_ModelBounds.SizeX, Math.Max(m_ModelBounds.SizeY, m_ModelBounds.SizeZ));
        m_CameraDistance = Math.Max(0.1, largestSize * 1.35);
        m_CameraYaw = 0;
        m_CameraPitch = 0.05;
        UpdateCamera();
    }

    private void UpdateCamera()
    {
        double horizontalDistance = m_CameraDistance * Math.Cos(m_CameraPitch);
        Point3D position = new(
            m_CameraTarget.X + horizontalDistance * Math.Sin(m_CameraYaw),
            m_CameraTarget.Y + m_CameraDistance * Math.Sin(m_CameraPitch),
            m_CameraTarget.Z + horizontalDistance * Math.Cos(m_CameraYaw));
        PerspectiveCamera camera = (PerspectiveCamera)Viewport.Camera;
        camera.Position = position;
        camera.LookDirection = m_CameraTarget - position;
        camera.UpDirection = new Vector3D(0, 1, 0);
    }

    private static Vector3 ApplyBindPose(RuntimeSkinVertex vertex, RuntimeSkinModel model)
    {
        Vector3 result = Vector3.Zero;
        float total = 0;
        for (int i = 0; i < vertex.BoneWeights.Length; ++i)
        {
            float weight = vertex.BoneWeights[i];
            if (weight <= 0.00001f) { continue; }
            int slotIndex = vertex.BoneIndices[i];
            if (slotIndex >= model.SkinSlots.Count) { continue; }
            Matrix4x4 world = GetBindWorld(model, model.SkinSlots[slotIndex].BoneIndex);
            result += Vector3.Transform(vertex.Position, model.SkinSlots[slotIndex].OffsetMatrix * world) * weight;
            total += weight;
        }
        return total > 0.00001f ? result / total : vertex.Position;
    }

    private static Matrix4x4 GetBindWorld(RuntimeSkinModel model, int boneIndex)
    {
        Matrix4x4 world = Matrix4x4.CreateScale(model.Bones[boneIndex].BindScale) * Matrix4x4.CreateFromQuaternion(model.Bones[boneIndex].BindRotation) * Matrix4x4.CreateTranslation(model.Bones[boneIndex].BindPosition);
        int parent = model.Bones[boneIndex].ParentIndex;
        return parent >= 0 ? world * GetBindWorld(model, parent) : world;
    }

    private FrameworkElement CreateMaterialPanel(RuntimeMaterial material, string sourcePath)
    {
        TextBlock details = new()
        {
            Text = FormatMaterial(material),
            Foreground = new SolidColorBrush(Color.FromRgb(200, 200, 200)),
            TextWrapping = TextWrapping.Wrap,
            LineHeight = 18,
            Margin = new Thickness(10, 8, 6, 6),
        };
        return new Expander
        {
            Header = Path.GetFileName(sourcePath),
            ToolTip = sourcePath,
            IsExpanded = false,
            Content = details,
            Padding = new Thickness(8, 7, 8, 7),
            Background = new SolidColorBrush(Color.FromRgb(45, 45, 48)),
            BorderBrush = new SolidColorBrush(Color.FromRgb(63, 63, 70)),
            BorderThickness = new Thickness(1),
        };
    }

    private void SetFileHeader(string filePath)
    {
        FileNameText.Text = Path.GetFileName(filePath);
        FileNameText.ToolTip = filePath;
        FilePathText.Text = Path.GetDirectoryName(filePath) ?? string.Empty;
        FilePathText.ToolTip = filePath;
    }

    private static string FormatMaterial(RuntimeMaterial material) => $"Diffuse: {material.Diffuse.X:0.###}, {material.Diffuse.Y:0.###}, {material.Diffuse.Z:0.###}, {material.Diffuse.W:0.###}\nSpecular: {material.Specular.X:0.###}, {material.Specular.Y:0.###}, {material.Specular.Z:0.###} (power {material.SpecularPower:0.###})\nAmbient: {material.Ambient.X:0.###}, {material.Ambient.Y:0.###}, {material.Ambient.Z:0.###}\nToon: {material.UseToonMap}, Sphere: {material.UseSphereMap}\nBase: {material.BaseColorTexturePath}\nNormal: {material.NormalMapTexturePath}\nToon texture: {material.ToonTexturePath}\nSphere texture: {material.SphereTexturePath}";

    private static DiffuseMaterial CreateDiffuseMaterial(RuntimeMaterial material, string modelFilePath, out bool textureResolved)
    {
        Color color = Color.FromArgb((byte)(Clamp(material.Diffuse.W) * 255), (byte)(Clamp(material.Diffuse.X) * 255), (byte)(Clamp(material.Diffuse.Y) * 255), (byte)(Clamp(material.Diffuse.Z) * 255));
        Brush brush = new SolidColorBrush(color);
        string texture = ResolveTexturePath(material.BaseColorTexturePath, modelFilePath);
        textureResolved = File.Exists(texture);
        if (textureResolved)
        {
            BitmapImage image = new();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.UriSource = new Uri(texture, UriKind.Absolute);
            image.EndInit();
            image.Freeze();
            brush = new ImageBrush(image) { Stretch = Stretch.Fill };
        }
        return new DiffuseMaterial(brush);
    }

    private static float Clamp(float value) => Math.Min(1, Math.Max(0, value));

    private static string ResolveTexturePath(string path, string modelFilePath)
    {
        if (string.IsNullOrWhiteSpace(path)) { return string.Empty; }
        string normalized = path.Replace('/', Path.DirectorySeparatorChar);
        if (Path.IsPathRooted(normalized) && File.Exists(normalized)) { return Path.GetFullPath(normalized); }

        DirectoryInfo? directory = Directory.GetParent(Path.GetFullPath(modelFilePath));
        while (directory is not null)
        {
            string candidate = Path.Combine(directory.FullName, normalized);
            if (File.Exists(candidate)) { return Path.GetFullPath(candidate); }
            directory = directory.Parent;
        }
        return string.Empty;
    }

    private void ClearModel()
    {
        ModelVisual.Content = null;
        MaterialsList.ItemsSource = null;
        SummaryText.Text = string.Empty;
    }
}
