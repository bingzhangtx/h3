/*
 * Copyright 2017 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdlib.h>
#include "bbox.h"
#include "constants.h"
#include "geoCoord.h"
#include "h3Index.h"
#include "linkedGeo.h"
#include "polygon.h"
#include "test.h"

// Fixtures
GeoCoord sfVerts[] = {
    {0.659966917655, -2.1364398519396},  {0.6595011102219, -2.1359434279405},
    {0.6583348114025, -2.1354884206045}, {0.6581220034068, -2.1382437718946},
    {0.6594479998527, -2.1384597563896}, {0.6599990002976, -2.1376771158464}};
Geofence sfGeofence;

GeoCoord primeMeridianVerts[] = {
    {0.01, 0.01}, {0.01, -0.01}, {-0.01, -0.01}, {-0.01, 0.01}};
Geofence primeMeridianGeofence;

GeoCoord transMeridianVerts[] = {{0.01, -M_PI + 0.01},
                                 {0.01, M_PI - 0.01},
                                 {-0.01, M_PI - 0.01},
                                 {-0.01, -M_PI + 0.01}};
Geofence transMeridianGeofence;

GeoCoord transMeridianHoleVerts[] = {{0.005, -M_PI + 0.005},
                                     {0.005, M_PI - 0.005},
                                     {-0.005, M_PI - 0.005},
                                     {-0.005, -M_PI + 0.005}};
Geofence transMeridianHoleGeofence;

static void createLinkedLoop(LinkedGeoLoop* loop, GeoCoord* verts, int numVerts) {
    initLinkedLoop(loop);
    for (int i = 0; i < numVerts; i++) {
        addLinkedCoord(loop, verts++);
    }
}

BEGIN_TESTS(polygon);

sfGeofence.numVerts = 6;
sfGeofence.verts = sfVerts;

primeMeridianGeofence.numVerts = 4;
primeMeridianGeofence.verts = primeMeridianVerts;

transMeridianGeofence.numVerts = 4;
transMeridianGeofence.verts = transMeridianVerts;

transMeridianHoleGeofence.numVerts = 4;
transMeridianHoleGeofence.verts = transMeridianHoleVerts;

TEST(pointInsideGeofence) {
    GeoCoord somewhere = {1, 2};

    BBox bbox;
    bboxFromGeofence(&sfGeofence, &bbox);

    t_assert(!pointInsideGeofence(&sfGeofence, &bbox, &sfVerts[0]),
             "contains exact");
    t_assert(pointInsideGeofence(&sfGeofence, &bbox, &sfVerts[4]),
             "contains exact4");
    t_assert(!pointInsideGeofence(&sfGeofence, &bbox, &somewhere),
             "contains somewhere else");
}

TEST(pointInsideGeofenceTransmeridian) {
    GeoCoord eastPoint = {0.001, -M_PI + 0.001};
    GeoCoord eastPointOutside = {0.001, -M_PI + 0.1};
    GeoCoord westPoint = {0.001, M_PI - 0.001};
    GeoCoord westPointOutside = {0.001, M_PI - 0.1};

    BBox bbox;
    bboxFromGeofence(&transMeridianGeofence, &bbox);

    t_assert(pointInsideGeofence(&transMeridianGeofence, &bbox, &westPoint),
             "contains point to the west of the antimeridian");
    t_assert(pointInsideGeofence(&transMeridianGeofence, &bbox, &eastPoint),
             "contains point to the east of the antimeridian");
    t_assert(
        !pointInsideGeofence(&transMeridianGeofence, &bbox, &westPointOutside),
        "does not contain outside point to the west of the antimeridian");
    t_assert(
        !pointInsideGeofence(&transMeridianGeofence, &bbox, &eastPointOutside),
        "does not contain outside point to the east of the antimeridian");
}

TEST(pointInsideLinkedGeoLoop) {
    GeoCoord somewhere = {1, 2};
    GeoCoord inside = {0.659, -2.136};

    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &sfVerts[0], 6);

    BBox bbox;
    bboxFromLinkedGeoLoop(&loop, &bbox);

    t_assert(pointInsideLinkedGeoLoop(&loop, &bbox, &inside),
             "contains exact4");
    t_assert(!pointInsideLinkedGeoLoop(&loop, &bbox, &somewhere),
             "contains somewhere else");

    destroyLinkedGeoLoop(&loop);
}

TEST(bboxFromGeofence) {
    GeoCoord verts[] = {{0.8, 0.3}, {0.7, 0.6}, {1.1, 0.7}, {1.0, 0.2}};
    Geofence geofence = {.numVerts = 4, .verts = verts};

    const BBox expected = {1.1, 0.7, 0.7, 0.2};

    BBox result;
    bboxFromGeofence(&geofence, &result);
    t_assert(bboxEquals(&result, &expected), "Got expected bbox");
}

TEST(bboxFromGeofenceTransmeridian) {
    GeoCoord verts[] = {{0.1, -M_PI + 0.1},  {0.1, M_PI - 0.1},
                        {0.05, M_PI - 0.2},  {-0.1, M_PI - 0.1},
                        {-0.1, -M_PI + 0.1}, {-0.05, -M_PI + 0.2}};
    Geofence geofence = {.numVerts = 6, .verts = verts};

    const BBox expected = {0.1, -0.1, -M_PI + 0.2, M_PI - 0.2};

    BBox result;
    bboxFromGeofence(&geofence, &result);
    t_assert(bboxEquals(&result, &expected), "Got expected transmeridian bbox");
}

TEST(bboxFromGeofenceNoVertices) {
    Geofence geofence;
    geofence.verts = NULL;
    geofence.numVerts = 0;

    const BBox expected = {0.0, 0.0, 0.0, 0.0};

    BBox result;
    bboxFromGeofence(&geofence, &result);

    t_assert(bboxEquals(&result, &expected), "Got expected bbox");
}

TEST(bboxesFromGeoPolygon) {
    GeoCoord verts[] = {{0.8, 0.3}, {0.7, 0.6}, {1.1, 0.7}, {1.0, 0.2}};
    Geofence geofence = {.numVerts = 4, .verts = verts};
    GeoPolygon polygon = {.geofence = geofence, .numHoles = 0};

    const BBox expected = {1.1, 0.7, 0.7, 0.2};

    BBox* result = calloc(sizeof(BBox), 1);
    bboxesFromGeoPolygon(&polygon, result);
    t_assert(bboxEquals(&result[0], &expected), "Got expected bbox");

    free(result);
}

TEST(bboxesFromGeoPolygonHole) {
    GeoCoord verts[] = {{0.8, 0.3}, {0.7, 0.6}, {1.1, 0.7}, {1.0, 0.2}};
    Geofence geofence = {.numVerts = 4, .verts = verts};

    // not a real hole, but doesn't matter for the test
    GeoCoord holeVerts[] = {{0.9, 0.3}, {0.9, 0.5}, {1.0, 0.7}, {0.9, 0.3}};
    Geofence holeGeofence = {.numVerts = 4, .verts = holeVerts};

    GeoPolygon polygon = {
        .geofence = geofence, .numHoles = 1, .holes = &holeGeofence};

    const BBox expected = {1.1, 0.7, 0.7, 0.2};
    const BBox expectedHole = {1.0, 0.9, 0.7, 0.3};

    BBox* result = calloc(sizeof(BBox), 2);
    bboxesFromGeoPolygon(&polygon, result);
    t_assert(bboxEquals(&result[0], &expected), "Got expected bbox");
    t_assert(bboxEquals(&result[1], &expectedHole), "Got expected hole bbox");

    free(result);
}

TEST(bboxFromLinkedGeoLoop) {
    GeoCoord verts[] = {{0.8, 0.3}, {0.7, 0.6}, {1.1, 0.7}, {1.0, 0.2}};

    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &verts[0], 4);

    const BBox expected = {1.1, 0.7, 0.7, 0.2};

    BBox result;
    bboxFromLinkedGeoLoop(&loop, &result);
    t_assert(bboxEquals(&result, &expected), "Got expected bbox");

    destroyLinkedGeoLoop(&loop);
}

TEST(bboxFromLinkedGeoLoopNoVertices) {
    LinkedGeoLoop loop;
    initLinkedLoop(&loop);

    const BBox expected = {0.0, 0.0, 0.0, 0.0};

    BBox result;
    bboxFromLinkedGeoLoop(&loop, &result);

    t_assert(bboxEquals(&result, &expected), "Got expected bbox");

    destroyLinkedGeoLoop(&loop);
}

TEST(isClockwiseGeofence) {
    GeoCoord verts[] = {{0, 0}, {0.1, 0.1}, {0, 0.1}};
    Geofence geofence = {.numVerts = 3, .verts = verts};

    t_assert(isClockwiseGeofence(&geofence), "Got true for clockwise geofence");
}

TEST(isClockwiseLinkedGeoLoop) {
    GeoCoord verts[] = {{0.1, 0.1}, {0.2, 0.2}, {0.1, 0.2}};
    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &verts[0], 3);

    t_assert(isClockwiseLinkedGeoLoop(&loop), "Got true for clockwise loop");

    destroyLinkedGeoLoop(&loop);
}

TEST(isNotClockwiseLinkedGeoLoop) {
    GeoCoord verts[] = {{0, 0}, {0, 0.4}, {0.4, 0.4}, {0.4, 0}};
    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &verts[0], 4);

    t_assert(!isClockwiseLinkedGeoLoop(&loop),
             "Got false for counter-clockwise loop");

    destroyLinkedGeoLoop(&loop);
}

TEST(isClockwiseGeofenceTransmeridian) {
    GeoCoord verts[] = {{0.4, M_PI - 0.1},
                        {0.4, -M_PI + 0.1},
                        {-0.4, -M_PI + 0.1},
                        {-0.4, M_PI - 0.1}};
     Geofence geofence = {.numVerts = 4, .verts = verts};

    t_assert(isClockwiseGeofence(&geofence), "Got true for clockwise geofence");
}

TEST(isClockwiseLinkedGeoLoopTransmeridian) {
    GeoCoord verts[] = {{0.4, M_PI - 0.1},
                        {0.4, -M_PI + 0.1},
                        {-0.4, -M_PI + 0.1},
                        {-0.4, M_PI - 0.1}};
    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &verts[0], 4);

    t_assert(isClockwiseLinkedGeoLoop(&loop) == true,
             "Got true for clockwise transmeridian loop");

    destroyLinkedGeoLoop(&loop);
}

TEST(isNotClockwiseLinkedGeoLoopTransmeridian) {
    GeoCoord verts[] = {{0.4, M_PI - 0.1},
                        {-0.4, M_PI - 0.1},
                        {-0.4, -M_PI + 0.1},
                        {0.4, -M_PI + 0.1}};
    LinkedGeoLoop loop;
    createLinkedLoop(&loop, &verts[0], 4);

    t_assert(isClockwiseLinkedGeoLoop(&loop) == false,
             "Got false for counter-clockwise transmeridian loop");

    destroyLinkedGeoLoop(&loop);
}

TEST(normalizeMultiPolygonSingle) {
    GeoCoord verts[] = {{0, 0}, {0, 1}, {1, 1}};

    LinkedGeoLoop* outer = calloc(1, sizeof(*outer));
    assert(outer != NULL);
    createLinkedLoop(outer, &verts[0], 3);

    LinkedGeoPolygon polygon;
    initLinkedPolygon(&polygon);
    addLinkedLoop(&polygon, outer);

    normalizeMultiPolygon(&polygon);

    t_assert(countLinkedPolygons(&polygon) == 1, "Polygon count correct");
    t_assert(countLinkedLoops(&polygon) == 1, "Loop count correct");
    t_assert(polygon.first == outer, "Got expected loop");

    H3_EXPORT(destroyLinkedPolygon)(&polygon);
}

TEST(normalizeMultiPolygonTwoOuterLoops) {
    GeoCoord verts1[] = {{0, 0}, {0, 1}, {1, 1}};

    LinkedGeoLoop* outer1 = calloc(1, sizeof(*outer1));
    assert(outer1 != NULL);
    createLinkedLoop(outer1, &verts1[0], 3);

    GeoCoord verts2[] = {{2, 2}, {2, 3}, {3, 3}};

    LinkedGeoLoop* outer2 = calloc(1, sizeof(*outer2));
    assert(outer2 != NULL);
    createLinkedLoop(outer2, &verts2[0], 3);

    LinkedGeoPolygon polygon;
    initLinkedPolygon(&polygon);
    addLinkedLoop(&polygon, outer1);
    addLinkedLoop(&polygon, outer2);

    normalizeMultiPolygon(&polygon);

    t_assert(countLinkedPolygons(&polygon) == 2, "Polygon count correct");
    t_assert(countLinkedLoops(&polygon) == 1,
             "Loop count on first polygon correct");
    t_assert(countLinkedLoops(polygon.next) == 1,
             "Loop count on second polygon correct");

    H3_EXPORT(destroyLinkedPolygon)(&polygon);
}

TEST(normalizeMultiPolygonOneHole) {
    GeoCoord verts[] = {{0, 0}, {0, 3}, {3, 3}, {3, 0}};

    LinkedGeoLoop* outer = calloc(1, sizeof(*outer));
    assert(outer != NULL);
    createLinkedLoop(outer, &verts[0], 4);

    GeoCoord verts2[] = {{1, 1}, {2, 2}, {1, 2}};

    LinkedGeoLoop* inner = calloc(1, sizeof(*inner));
    assert(inner != NULL);
    createLinkedLoop(inner, &verts2[0], 3);

    LinkedGeoPolygon polygon;
    initLinkedPolygon(&polygon);
    addLinkedLoop(&polygon, inner);
    addLinkedLoop(&polygon, outer);

    normalizeMultiPolygon(&polygon);

    t_assert(countLinkedPolygons(&polygon) == 1, "Polygon count correct");
    t_assert(countLinkedLoops(&polygon) == 2,
             "Loop count on first polygon correct");
    t_assert(polygon.first == outer, "Got expected outer loop");
    t_assert(polygon.first->next == inner, "Got expected inner loop");

    H3_EXPORT(destroyLinkedPolygon)(&polygon);
}

TEST(normalizeMultiPolygonTwoHoles) {
    GeoCoord verts[] = {{0, 0}, {0, 0.4}, {0.4, 0.4}, {0.4, 0}};

    LinkedGeoLoop* outer = calloc(1, sizeof(*outer));
    assert(outer != NULL);
    createLinkedLoop(outer, &verts[0], 4);

    GeoCoord verts2[] = {{0.1, 0.1}, {0.2, 0.2}, {0.1, 0.2}};

    LinkedGeoLoop* inner1 = calloc(1, sizeof(*inner1));
    assert(inner1 != NULL);
    createLinkedLoop(inner1, &verts2[0], 3);

    GeoCoord verts3[] = {{0.2, 0.2}, {0.3, 0.3}, {0.2, 0.3}};

    LinkedGeoLoop* inner2 = calloc(1, sizeof(*inner2));
    assert(inner2 != NULL);
    createLinkedLoop(inner2, &verts3[0], 3);

    LinkedGeoPolygon polygon;
    initLinkedPolygon(&polygon);
    addLinkedLoop(&polygon, inner2);
    addLinkedLoop(&polygon, outer);
    addLinkedLoop(&polygon, inner1);

    normalizeMultiPolygon(&polygon);

    t_assert(countLinkedPolygons(&polygon) == 1,
             "Polygon count correct for 2 holes");
    t_assert(polygon.first == outer, "Got expected outer loop");
    t_assert(countLinkedLoops(&polygon) == 3,
             "Loop count on first polygon correct");

    H3_EXPORT(destroyLinkedPolygon)(&polygon);
}

TEST(normalizeMultiPolygonNoOuterLoops) {
    GeoCoord verts1[] = {{0, 0}, {1, 1}, {0, 1}};

    LinkedGeoLoop* outer1 = calloc(1, sizeof(*outer1));
    assert(outer1 != NULL);
    createLinkedLoop(outer1, &verts1[0], 3);

    GeoCoord verts2[] = {{2, 2}, {3, 3}, {2, 3}};

    LinkedGeoLoop* outer2 = calloc(1, sizeof(*outer2));
    assert(outer2 != NULL);
    createLinkedLoop(outer2, &verts2[0], 3);

    LinkedGeoPolygon polygon;
    initLinkedPolygon(&polygon);
    addLinkedLoop(&polygon, outer1);
    addLinkedLoop(&polygon, outer2);

    normalizeMultiPolygon(&polygon);

    t_assert(countLinkedPolygons(&polygon) == 1, "Polygon count correct");
    t_assert(countLinkedLoops(&polygon) == 0,
             "Loop count as expected with invalid input");

    // Note: These got de-linked so we need to free them separately
    destroyLinkedGeoLoop(outer1);
    destroyLinkedGeoLoop(outer2);
}

END_TESTS();
