/*
   Copyright (c) 2013, Sean Kasun
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#include <QMap>
#include <QString>
#include <QVariant>
#include <QPainter>
class Tag;

class Structure
{
public:
	Structure(int nx1 = 0, int ny1 = 0, int nz1 = 0,
			  int nx2 = 0, int ny2 = 0, int nz2 = 0,
			  QString nid = "");

	Structure(int nx1, int ny1, int nz1,
			  int nx2, int ny2, int nz2,
			  QString  nid,
			  QVariant npropperties);

	bool  load(const QVariant& feature);

	const QString&  getId() const { return id; }
	const QVariant& getProperties() const { return properties; }
	double getX1() const { return x1; }
	double getY1() const { return y1; }
	double getZ1() const { return z1; }
	double getX2() const { return x2; }
	double getY2() const { return y2; }
	double getZ2() const { return z2; }

	void draw(double offsetX, double offsetZ, double scale, QPainter& canvas) const;
	
	bool intersects(double x1, double y1, double z1,
                    double x2, double y2, double z2) const;
	
private:
	static const int MIN_SIZE = 10;

	int      x1, y1, z1, x2, y2, z2;
	QString  id;
	QVariant properties;
	QColor   color;
};


//           type      
typedef QMap<QString, QList< Structure > > TStructureMap;

#endif
