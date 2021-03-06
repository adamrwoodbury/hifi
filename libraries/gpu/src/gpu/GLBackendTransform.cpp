//
//  GLBackendTransform.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

#include "Format.h"

using namespace gpu;

// Transform Stage

void GLBackend::do_setModelTransform(Batch& batch, uint32 paramOffset) {
    _transform._model = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidModel = true;
}

void GLBackend::do_setViewTransform(Batch& batch, uint32 paramOffset) {
    _transform._view = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidView = true;
}

void GLBackend::do_setProjectionTransform(Batch& batch, uint32 paramOffset) {
    memcpy(&_transform._projection, batch.editData(batch._params[paramOffset]._uint), sizeof(Mat4));
    _transform._invalidProj = true;
}

void GLBackend::initTransform() {
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    glGenBuffers(1, &_transform._transformObjectBuffer);
    glGenBuffers(1, &_transform._transformCameraBuffer);

    glBindBuffer(GL_UNIFORM_BUFFER, _transform._transformObjectBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_transform._transformObject), (const void*) &_transform._transformObject, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, _transform._transformCameraBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(_transform._transformCamera), (const void*) &_transform._transformCamera, GL_DYNAMIC_DRAW);


    glBindBuffer(GL_UNIFORM_BUFFER, 0);
#else
#endif
}

void GLBackend::killTransform() {
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    glDeleteBuffers(1, &_transform._transformObjectBuffer);
    glDeleteBuffers(1, &_transform._transformCameraBuffer);
#else
#endif
}
void GLBackend::updateTransform() {
    // Check all the dirty flags and update the state accordingly
    if (_transform._invalidProj) {
        _transform._transformCamera._projection = _transform._projection;
    }

    if (_transform._invalidView) {
        _transform._view.getInverseMatrix(_transform._transformCamera._view);
        _transform._view.getMatrix(_transform._transformCamera._viewInverse);
    }

    if (_transform._invalidModel) {
        _transform._model.getMatrix(_transform._transformObject._model);
        _transform._model.getInverseMatrix(_transform._transformObject._modelInverse);
    }

    if (_transform._invalidView || _transform._invalidProj) {
        Mat4 viewUntranslated = _transform._transformCamera._view;
        viewUntranslated[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        _transform._transformCamera._projectionViewUntranslated = _transform._transformCamera._projection * viewUntranslated;
    }
 
 #if (GPU_TRANSFORM_PROFILE == GPU_CORE)
    if (_transform._invalidView || _transform._invalidProj) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _transform._transformCameraBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_transform._transformCamera), (const void*) &_transform._transformCamera, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
   }

    if (_transform._invalidModel) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, _transform._transformObjectBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_transform._transformObject), (const void*) &_transform._transformObject, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_OBJECT_SLOT, _transform._transformObjectBuffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, _transform._transformCameraBuffer);
    CHECK_GL_ERROR();
#endif


#if (GPU_TRANSFORM_PROFILE == GPU_LEGACY)
    // Do it again for fixed pipeline until we can get rid of it
    if (_transform._invalidProj) {
        if (_transform._lastMode != GL_PROJECTION) {
            glMatrixMode(GL_PROJECTION);
            _transform._lastMode = GL_PROJECTION;
        }
        glLoadMatrixf(reinterpret_cast< const GLfloat* >(&_transform._projection));

        (void) CHECK_GL_ERROR();
    }

    if (_transform._invalidModel || _transform._invalidView) {
        if (!_transform._model.isIdentity()) {
            if (_transform._lastMode != GL_MODELVIEW) {
                glMatrixMode(GL_MODELVIEW);
                _transform._lastMode = GL_MODELVIEW;
            }
            Transform::Mat4 modelView;
            if (!_transform._view.isIdentity()) {
                Transform mvx;
                Transform::inverseMult(mvx, _transform._view, _transform._model);
                mvx.getMatrix(modelView);
            } else {
                _transform._model.getMatrix(modelView);
            }
            glLoadMatrixf(reinterpret_cast< const GLfloat* >(&modelView));
        } else {
            if (!_transform._view.isIdentity()) {
                if (_transform._lastMode != GL_MODELVIEW) {
                    glMatrixMode(GL_MODELVIEW);
                    _transform._lastMode = GL_MODELVIEW;
                }
                Transform::Mat4 modelView;
                _transform._view.getInverseMatrix(modelView);
                glLoadMatrixf(reinterpret_cast< const GLfloat* >(&modelView));
            } else {
                // TODO: eventually do something about the matrix when neither view nor model is specified?
                // glLoadIdentity();
            }
        }
        (void) CHECK_GL_ERROR();
    }
#endif

    // Flags are clean
    _transform._invalidView = _transform._invalidProj = _transform._invalidModel = false;
}


