// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
/// \file main.cpp
// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

#include "D3DApp.h"
#include "d3dx11Effect.h"
#include "MathHelper.h"

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

class App : public D3DApp {

public:

    App( HINSTANCE hInst );
    virtual ~App( void ) override;

    virtual bool init( void ) override;
    virtual void onResize( void ) override;
    virtual void updateScene( const float dt ) override;
    virtual void drawScene( void ) override;

    virtual void onMouseDown( WPARAM btnState, int x, int y ) override;
    virtual void onMouseUp( WPARAM btnState, int x, int y ) override;
    virtual void onMouseMove( WPARAM btnState, int x, int y ) override;

private:

    void buildGeometryBuffers( void );
    void buildFX( void );
    void buildVertexLayout( void );

private:

    ID3D11Buffer* mBoxVB;
    ID3D11Buffer* mBoxIB;

    ID3DX11Effect* mFX;
    ID3DX11EffectTechnique* mTech;
    ID3DX11EffectMatrixVariable* mfxWorldViewProj;

    ID3D11InputLayout* mInputLayout;

    ID3D11RasterizerState* mWireframeRS;

    DirectX::XMFLOAT4X4 mSkullWorld, mView, mProj;

    UINT mSkullIndexCount;

    float mTheta, mPhi, mRadius;

    POINT mLastMousePos;

};

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE prevInstance,
                    PSTR cmdLine, int showCmd )
{
#if defined( DEBUG ) || defined( _DEBUG )
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    App app( hInstance );

    if ( !app.init() ) {
        return 0;
    }

    return app.run();
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

App::App( HINSTANCE hInstance )
    : D3DApp( hInstance )
    , mBoxVB( nullptr )
    , mBoxIB( nullptr )
    , mFX( nullptr )
    , mTech( nullptr )
    , mfxWorldViewProj( nullptr )
    , mInputLayout( nullptr )
    , mTheta( 1.5f * MathHelper::Pi )
    , mPhi( 0.1f * MathHelper::Pi )
    , mRadius( 20.0f )
{
    mMainWindowCaption = L"Skull Demo";

    ZeroMemory( &mLastMousePos, sizeof( POINT ) );

    DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
    DirectX::XMStoreFloat4x4( &mSkullWorld, DirectX::XMMatrixTranslation( 0.f, -2.f, 0.f) );
    DirectX::XMStoreFloat4x4( &mView, I );
    DirectX::XMStoreFloat4x4( &mProj, I );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

App::~App( void )
{
    ReleaseCOM( mBoxVB );
    ReleaseCOM( mBoxIB );
    ReleaseCOM( mFX );
    ReleaseCOM( mInputLayout );
    ReleaseCOM( mWireframeRS );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

bool App::init( void )
{
    if ( !D3DApp::init() ) {
        return false;
    }

    buildGeometryBuffers();
    buildFX();
    buildVertexLayout();

    D3D11_RASTERIZER_DESC wireframeDesc;
    ZeroMemory( &wireframeDesc, sizeof( D3D11_RASTERIZER_DESC ) );
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_BACK;
    wireframeDesc.FrontCounterClockwise = false;
    wireframeDesc.DepthClipEnable = true;

    HR( mD3DDevice->CreateRasterizerState( &wireframeDesc, &mWireframeRS ) );

    return true;
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::onResize( void )
{
    D3DApp::onResize();

    // Update the aspect ratio and recompute the projection matrix.
    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH( 0.25f * MathHelper::Pi,
                                           getAspectRatio(),
                                           1.f,
                                           1000.f );
    XMStoreFloat4x4( &mProj, P );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::updateScene( const float dt )
{
    // Convert spherical to cartesian.
    float x = mRadius * sinf( mPhi ) * cosf( mTheta );
    float y = mRadius * sinf( mPhi ) * sinf( mTheta );
    float z = mRadius * cosf( mPhi );

    // Build the view matrix.
    DirectX::XMVECTOR pos = DirectX::XMVectorSet( x, y, z, 1.f );
    DirectX::XMVECTOR target = DirectX::XMVectorZero();
    DirectX::XMVECTOR up = DirectX::XMVectorSet( 0.f, 1.f, 0.f, 0.f );

    DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH( pos, target, up );
    XMStoreFloat4x4( &mView, V );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::drawScene( void )
{
    mD3DImmediateContext->ClearRenderTargetView( mRenderTargetView,
                                                 reinterpret_cast<const float*>( &Colors::LightSteelBlue ) );
    mD3DImmediateContext->ClearDepthStencilView( mDepthStencilView,
                                                 D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                                 1.f,
                                                 0 );

    mD3DImmediateContext->IASetInputLayout( mInputLayout );
    mD3DImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    mD3DImmediateContext->RSSetState( mWireframeRS );

    UINT stride = sizeof( Vertex );
    UINT offset = 0;
    mD3DImmediateContext->IASetVertexBuffers( 0,
                                              1,
                                              &mBoxVB,
                                              &stride,
                                              &offset );
    mD3DImmediateContext->IASetIndexBuffer( mBoxIB,
                                            DXGI_FORMAT_R32_UINT,
                                            0 );

    // Set constant buffers.
    DirectX::XMMATRIX world = XMLoadFloat4x4( &mSkullWorld );
    DirectX::XMMATRIX view = XMLoadFloat4x4( &mView );
    DirectX::XMMATRIX proj = XMLoadFloat4x4( &mProj );
    DirectX::XMMATRIX worldViewProj = world * view * proj;

    mfxWorldViewProj->SetMatrix( reinterpret_cast<float*>( &worldViewProj ) );

    D3DX11_TECHNIQUE_DESC techDesc;
    mTech->GetDesc( &techDesc );
    for ( UINT p = 0; p < techDesc.Passes; ++p ) {
        mTech->GetPassByIndex( p )->Apply( 0, mD3DImmediateContext );

        mD3DImmediateContext->DrawIndexed( mSkullIndexCount, 0, 0 );
    }

    HR( mSwapChain->Present( 0, 0 ) );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::onMouseDown( WPARAM btnState, int x, int y )
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture( mMainWindow );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::onMouseUp( WPARAM btnState, int x, int y )
{
    ReleaseCapture();
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::onMouseMove( WPARAM btnState, int x, int y )
{
    if ( ( btnState & MK_LBUTTON ) != 0 ) {
        // Correspond each pixel to a quarter of a degree.
        float dx = DirectX::XMConvertToRadians( 0.25f * static_cast<float>( x - mLastMousePos.x ) );
        float dy = DirectX::XMConvertToRadians( 0.25f * static_cast<float>( y - mLastMousePos.y ) );

        // Update spherical coordinates for angles.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle phi.
        mPhi = MathHelper::Clamp( mPhi, 0.1f, MathHelper::Pi - 0.1f );
    }
    else if ( ( btnState & MK_RBUTTON ) != 0 ) {
        // Correspond each pixel to 0.005 units in the scene.
        float dx = 0.05f * static_cast<float>( x - mLastMousePos.x );
        float dy = 0.05f * static_cast<float>( y - mLastMousePos.y );

        // Update camera radius.
        mRadius += dx - dy;

        // Restrict radius.
        mRadius = MathHelper::Clamp( mRadius, 5.f, 50.f );
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::buildGeometryBuffers( void )
{
    std::ifstream fin( "Models/skull.txt" );
    if ( !fin ) {
        MessageBox( nullptr, L"Models/skull.txt not found.", nullptr, 0 );
    }

    UINT vcount = 0, tcount = 0;
    std::string ignore;

    // VertexCount: 31076
    // TriangleCount: 60339
    // VertexList ( pos, normal )
    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    float nx, ny, nz;
    DirectX::XMFLOAT4 black( 0.f, 0.f, 0.f, 1.f );

    std::vector<Vertex> vertices( vcount );
    for ( UINT i = 0; i < vcount; ++i ) {
        fin >> vertices[i].pos.x >> vertices[i].pos.y >> vertices[i].pos.z;
        vertices[i].color = black;

        // Ignore normals for this demo.
        fin >> nx >> ny >> nz;
    }

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    mSkullIndexCount = 3 * tcount;
    std::vector<UINT> indices( mSkullIndexCount );
    for ( UINT i = 0; i < tcount; ++i ) {
        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
    }
    
    fin.close();

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof( bd ) );
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof( Vertex ) * vcount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory( &initData, sizeof( initData ) );
    initData.pSysMem = &vertices[0];
    HR( mD3DDevice->CreateBuffer( &bd, &initData, &mBoxVB ) );

    D3D11_BUFFER_DESC ibd;
    ZeroMemory( &ibd, sizeof( ibd ) );
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof( UINT ) * mSkullIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR( mD3DDevice->CreateBuffer( &ibd, &iinitData, &mBoxIB ) );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::buildFX( void )
{
    DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined ( _DEBUG )   
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* compiledShader = nullptr;
    ID3DBlob* compilationMsgs = nullptr;
    /*HRESULT hr = D3DX11CompileEffectFromFile( L"FX/color.fx",
                                        nullptr,
                                        D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                        shaderFlags,
                                        0,
                                        mD3DDevice,
                                        &mFX,
                                        &compilationMsgs );*/
    HRESULT hr = D3DCompileFromFile( L"FX/color.fx",
                                     nullptr,
                                     D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     nullptr,
                                     "fx_5_0",
                                     shaderFlags,
                                     0,
                                     &compiledShader,
                                     &compilationMsgs );

    if ( compilationMsgs != nullptr ) {
        // Filter out warning about deprecated compiler since effects are being used.
        if ( std::strstr( static_cast<const char*>( compilationMsgs->GetBufferPointer() ), 
                          "X4717" ) == 0 ) {
            MessageBoxA( 0,
                         static_cast<char*>( compilationMsgs->GetBufferPointer() ),
                         nullptr,
                         0 );
        }
        ReleaseCOM( compilationMsgs );
    }

    // Check for other errors.
    if ( FAILED( hr ) ) {
        DXTrace( __FILEW__,
        static_cast<DWORD>( __LINE__ ),
        hr,
        L"D3DX11CompileFromFile",
        true );
    }

    hr = D3DX11CreateEffectFromMemory( compiledShader->GetBufferPointer(),
                                      compiledShader->GetBufferSize(),
                                      0,
                                      mD3DDevice,
                                      &mFX );
    if ( FAILED( hr ) ) {
        DXTrace( __FILEW__,
                 static_cast<DWORD>( __LINE__ ),
                 hr,
                 L"D3DX11CreateEffectFromMemory",
                 true );
    }

    ReleaseCOM( compiledShader );

    mTech = mFX->GetTechniqueByName( "ColorTech" );
    // Store a pointer to constant buffer gWorldViewProj.
    mfxWorldViewProj = mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix();
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

void App::buildVertexLayout( void )
{
    // Create vertex input layout.
    D3D11_INPUT_ELEMENT_DESC vertexDesc [] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    D3DX11_PASS_DESC passDesc;
    mTech->GetPassByIndex( 0 )->GetDesc( &passDesc );
    HR( mD3DDevice->CreateInputLayout( vertexDesc,
                                       2,
                                       passDesc.pIAInputSignature,
                                       passDesc.IAInputSignatureSize,
                                       &mInputLayout ) );
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //